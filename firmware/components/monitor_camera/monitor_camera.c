#include "monitor_camera.h"

#include <limits.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_camera.h"
#include "esp_psram.h"
#include "esp_random.h"
#include "esp_timer.h"

static monitor_camera_status_t s_status = {
    .last_error = ESP_ERR_INVALID_STATE,
};
static camera_fb_t *s_buffer;
static monitor_camera_frame_t *s_owner;

static esp_err_t prepare_bus_selects(void)
{
    /* GPIO21 also drives the active-low LED. Never blink it as a heartbeat. */
    const gpio_num_t selects[] = {GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_21};
    for (size_t i = 0; i < sizeof(selects) / sizeof(selects[0]); ++i) {
        esp_err_t error = gpio_set_level(selects[i], 1);
        if (error != ESP_OK) {
            return error;
        }
    }
    /* Load the high output latches before enabling the drivers. */
    const gpio_config_t config = {
        .pin_bit_mask = (UINT64_C(1) << GPIO_NUM_2) |
                        (UINT64_C(1) << GPIO_NUM_3) |
                        (UINT64_C(1) << GPIO_NUM_21),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&config);
}

esp_err_t monitor_camera_init(void)
{
    if (s_status.initialization_attempted) {
        return ESP_ERR_INVALID_STATE;
    }
    s_status.initialization_attempted = true;
    esp_fill_random(s_status.boot_id, sizeof(s_status.boot_id));

    if (!esp_psram_is_initialized()) {
        s_status.last_error = ESP_ERR_NO_MEM;
        return s_status.last_error;
    }
    s_status.last_error = prepare_bus_selects();
    if (s_status.last_error != ESP_OK) {
        return s_status.last_error;
    }

    const camera_config_t config = {
        .pin_pwdn = -1,
        .pin_reset = -1,
        .pin_xclk = 10,
        .pin_sccb_sda = 40,
        .pin_sccb_scl = 39,
        .pin_d0 = 15,
        .pin_d1 = 17,
        .pin_d2 = 18,
        .pin_d3 = 16,
        .pin_d4 = 14,
        .pin_d5 = 12,
        .pin_d6 = 11,
        .pin_d7 = 48,
        .pin_vsync = 38,
        .pin_href = 47,
        .pin_pclk = 13,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 12,
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .sccb_i2c_port = 0,
    };
    s_status.last_error = esp_camera_init(&config);
    if (s_status.last_error != ESP_OK) {
        return s_status.last_error;
    }

    const sensor_t *sensor = esp_camera_sensor_get();
    if (sensor != NULL) {
        s_status.sensor_pid = sensor->id.PID;
    }
    if (sensor == NULL || s_status.sensor_pid != OV3660_PID) {
        s_status.cleanup_error = esp_camera_deinit();
        s_status.last_error = ESP_ERR_NOT_SUPPORTED;
        return s_status.last_error;
    }
    s_status.ready = true;
    return ESP_OK;
}

static bool valid_timestamp(const camera_fb_t *buffer, int64_t now_us,
                            int64_t *capture_us)
{
    const int64_t seconds = buffer->timestamp.tv_sec;
    const int64_t micros = buffer->timestamp.tv_usec;
    if (seconds < 0 || seconds > (INT64_MAX - INT64_C(999999)) / INT64_C(1000000) ||
        micros < 0 || micros >= INT64_C(1000000)) {
        return false;
    }
    *capture_us = seconds * INT64_C(1000000) + micros;
    return *capture_us > s_status.last_capture_started_us &&
           now_us >= *capture_us &&
           now_us - *capture_us <= MONITOR_CAMERA_MAX_AGE_US;
}

esp_err_t monitor_camera_acquire(monitor_camera_frame_t *frame)
{
    if (frame == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (frame == s_owner) {
        return ESP_ERR_INVALID_STATE;
    }
    memset(frame, 0, sizeof(*frame));
    if (!s_status.ready || s_owner != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    ++s_status.attempts;
    camera_fb_t *buffer = esp_camera_fb_get();
    int64_t capture_us = 0;
    const int64_t now_us = esp_timer_get_time();
    esp_err_t error = ESP_OK;
    if (buffer == NULL) {
        error = ESP_ERR_TIMEOUT;
    } else if (buffer->buf == NULL || buffer->width != MONITOR_CAMERA_WIDTH ||
               buffer->height != MONITOR_CAMERA_HEIGHT ||
               buffer->format != PIXFORMAT_RGB565 ||
               buffer->len != MONITOR_CAMERA_FRAME_BYTES ||
               !valid_timestamp(buffer, now_us, &capture_us)) {
        error = ESP_ERR_INVALID_RESPONSE;
    }
    if (error != ESP_OK) {
        if (buffer != NULL) {
            esp_camera_fb_return(buffer);
        }
        ++s_status.acquisition_failures;
        s_status.last_error = error;
        return error;
    }

    s_buffer = buffer;
    s_owner = frame;
    s_status.frame_outstanding = true;
    ++s_status.frames_delivered;
    s_status.last_capture_started_us = capture_us;
    s_status.last_delivery_us = now_us;
    s_status.last_error = ESP_OK;
    *frame = (monitor_camera_frame_t) {
        .data = buffer->buf,
        .length = buffer->len,
        .width = buffer->width,
        .height = buffer->height,
        .format = MONITOR_CAMERA_RGB565,
        .sequence = s_status.frames_delivered,
        .capture_started_us = capture_us,
        .delivered_us = now_us,
    };
    memcpy(frame->boot_id, s_status.boot_id, sizeof(frame->boot_id));
    return ESP_OK;
}

esp_err_t monitor_camera_release(monitor_camera_frame_t *frame)
{
    if (frame == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (frame != s_owner || s_buffer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_camera_fb_return(s_buffer);
    s_buffer = NULL;
    s_owner = NULL;
    s_status.frame_outstanding = false;
    memset(frame, 0, sizeof(*frame));
    return ESP_OK;
}

esp_err_t monitor_camera_get_status(monitor_camera_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *status = s_status;
    return ESP_OK;
}
