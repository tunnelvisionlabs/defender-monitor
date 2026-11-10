#include <inttypes.h>
#include <stdio.h>

#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "monitor_camera.h"
#include "monitor_preview.h"

#if !CONFIG_SPIRAM || !CONFIG_SPIRAM_MODE_OCT || !CONFIG_ESPTOOLPY_FLASHSIZE_8MB || \
    !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#error "Camera profile requires fresh sdkconfig.defaults (8 MB flash, octal PSRAM, native USB console)."
#endif

static const char *TAG = "monitor";
static TaskHandle_t s_preview_task;

static void preview_task(void *argument)
{
    (void)argument;
    for (;;) {
        monitor_preview_step();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void report_startup(void)
{
    const esp_app_desc_t *app = esp_app_get_description();
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash_bytes = 0;
    const esp_err_t flash_error = esp_flash_get_size(NULL, &flash_bytes);
    char build_hash[17];
    for (size_t i = 0; i < 8; ++i) {
        snprintf(build_hash + 2 * i, 3, "%02x", app->app_elf_sha256[i]);
    }
    ESP_LOGI(TAG, "build=%s version=%s elf_sha256_prefix=%s idf=%s",
             app->project_name, app->version, build_hash, app->idf_ver);
    ESP_LOGI(TAG, "profile=xiao-esp32s3-sense/ov3660 chip_model=%d revision=%u cores=%u",
             (int)chip.model, (unsigned)chip.revision, (unsigned)chip.cores);
    ESP_LOGI(TAG, "flash_bytes=%" PRIu32 " flash_status=%s psram_bytes=%u "
             "free_internal=%u free_psram=%u reset_reason=%d",
             flash_bytes, esp_err_to_name(flash_error), (unsigned)esp_psram_get_size(),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM), (int)esp_reset_reason());
}

void app_main(void)
{
    report_startup();
    const esp_err_t init_error = monitor_camera_init();
    monitor_camera_status_t status;
    monitor_camera_get_status(&status);
    char boot_id[2 * MONITOR_CAMERA_BOOT_ID_BYTES + 1];
    for (size_t i = 0; i < MONITOR_CAMERA_BOOT_ID_BYTES; ++i) {
        snprintf(boot_id + 2 * i, 3, "%02x", status.boot_id[i]);
    }
    ESP_LOGI(TAG, "boot_session=%s sensor_pid=0x%04x camera_status=%s cleanup_status=%s",
             boot_id, status.sensor_pid, esp_err_to_name(init_error),
             esp_err_to_name(status.cleanup_error));
    const esp_err_t preview_error = monitor_preview_init();
    bool preview_enabled = preview_error == ESP_OK;
    if (preview_enabled && xTaskCreate(preview_task, "preview", 6144, NULL, 4,
                                      &s_preview_task) != pdPASS) {
        preview_enabled = false;
        ESP_LOGE(TAG, "preview task allocation failed; capture continues without a screen");
    }
    ESP_LOGI(TAG, "preview_status=%s preview_enabled=%d lcd_target=240x240 spi_target_hz=20000000 "
             "preview_target=192x144 slots=3 lcd_canvas_bytes=115200",
             esp_err_to_name(preview_error), preview_enabled);
    if (init_error != ESP_OK) {
        ESP_LOGE(TAG, "capture stopped: check the recorded board/camera profile and PSRAM");
        if (preview_enabled) monitor_preview_capture_failed(init_error);
    }
    ESP_LOGI(TAG, "capture_target=%ux%u RGB565 buffers=1 max_frame_age_us=%" PRId64,
             MONITOR_CAMERA_WIDTH, MONITOR_CAMERA_HEIGHT, MONITOR_CAMERA_MAX_AGE_US);

    int64_t previous_report_us = esp_timer_get_time();
    uint64_t previous_frames = 0;
    for (;;) {
        monitor_camera_frame_t frame = {0};
        const esp_err_t error = init_error == ESP_OK ? monitor_camera_acquire(&frame) : init_error;
        if (error == ESP_OK) {
            if (preview_enabled) {
                const esp_err_t submit_error = monitor_preview_submit(&frame);
                if (submit_error != ESP_OK) {
                    ESP_LOGW(TAG, "preview submission failed: %s", esp_err_to_name(submit_error));
                }
            }
            /* Preview owns a bounded copy before this SDK lease is returned. */
            const esp_err_t release_error = monitor_camera_release(&frame);
            if (release_error != ESP_OK) {
                ESP_LOGE(TAG, "capture stopped: buffer release failed: %s",
                         esp_err_to_name(release_error));
                if (preview_enabled) monitor_preview_capture_failed(release_error);
                return;
            }
        } else {
            if (preview_enabled) monitor_preview_capture_failed(error);
            if (init_error == ESP_OK) {
                ESP_LOGW(TAG, "acquisition failed: %s (no fresh frame)", esp_err_to_name(error));
            }
        }
        const int64_t now_us = esp_timer_get_time();
        if (now_us - previous_report_us >= INT64_C(5000000)) {
            monitor_camera_get_status(&status);
            const int64_t age_us = status.frames_delivered == 0 ? -1 :
                                  now_us - status.last_capture_started_us;
            ESP_LOGI(TAG, "frames=%" PRIu64 " failures=%" PRIu64 " last_source_us=%" PRId64
                     " age_us=%" PRId64 " interval_us=%" PRId64 " interval_frames=%" PRIu64
                     " free_internal=%u min_internal=%u free_psram=%u task_stack_free_bytes=%u",
                     status.frames_delivered, status.acquisition_failures,
                     status.last_capture_started_us, age_us, now_us - previous_report_us,
                     status.frames_delivered - previous_frames,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                     (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                     (unsigned)uxTaskGetStackHighWaterMark(NULL));
            if (preview_enabled) {
                monitor_preview_status_t preview;
                monitor_preview_get_status(&preview);
                ESP_LOGI(TAG, "preview_submitted=%" PRIu64 " replaced=%" PRIu64 " expired=%" PRIu64
                         " rendered=%" PRIu64 " draw_failures=%" PRIu64 " draw_timeouts=%" PRIu64
                         " source_us=%" PRId64 " max_submit_us=%" PRId64 " display_ready=%d "
                         "pending=%d state=%d display_error=%s preview_stack_free_bytes=%u",
                         preview.submitted, preview.replaced, preview.expired, preview.rendered,
                         preview.draw_failures, preview.draw_timeouts, preview.last_rendered_capture_us,
                         preview.max_submit_us, preview.display_ready, preview.transfer_pending,
                         (int)preview.state, esp_err_to_name(preview.display_error),
                         (unsigned)uxTaskGetStackHighWaterMark(s_preview_task));
            }
            previous_report_us = now_us;
            previous_frames = status.frames_delivered;
        }
        vTaskDelay(pdMS_TO_TICKS(init_error == ESP_OK ? 100 : 1000));
    }
}
