#include "monitor_preview.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "preview_lcd.h"
#include "preview_text.h"

#define IMAGE_BYTES (MONITOR_PREVIEW_WIDTH * MONITOR_PREVIEW_HEIGHT * 2U)
#define CANVAS_BYTES (MONITOR_PREVIEW_SCREEN_SIZE * MONITOR_PREVIEW_SCREEN_SIZE * 2U)
#define DRAW_TIMEOUT_US INT64_C(500000)
#define INIT_RETRY_US INT64_C(5000000)

typedef enum { SLOT_FREE, SLOT_WRITING, SLOT_PENDING, SLOT_READING } slot_state_t;
typedef struct {
    slot_state_t state;
    uint8_t *pixels;
    uint64_t sequence;
    int64_t source_us;
    uint8_t boot_id[MONITOR_CAMERA_BOOT_ID_BYTES];
} preview_slot_t;

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static monitor_preview_status_t s_status;
static preview_slot_t s_slots[MONITOR_PREVIEW_SLOTS];
static uint8_t *s_images;
static uint8_t *s_canvas;
static bool s_init_attempted;
static bool s_has_source;
static uint8_t s_boot_id[MONITOR_CAMERA_BOOT_ID_BYTES];
static uint64_t s_sequence;
static int64_t s_source_us;
/* The fields below are display-owner only. */
static bool s_lcd_ready;
static bool s_transfer;
static bool s_timeout_reported;
static int64_t s_draw_started_us;
static int64_t s_next_init_us;
static int64_t s_next_status_us;
static monitor_preview_state_t s_drawn_state;
static uint64_t s_drawn_sequence;
static int64_t s_drawn_source_us;
static uint8_t s_drawn_boot_id[MONITOR_CAMERA_BOOT_ID_BYTES];

esp_err_t monitor_preview_init(void)
{
    if (s_init_attempted) return ESP_ERR_INVALID_STATE;
    s_init_attempted = true;
    s_images = heap_caps_malloc(MONITOR_PREVIEW_SLOTS * IMAGE_BYTES,
                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_canvas = heap_caps_malloc(CANVAS_BYTES,
                               MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (s_images == NULL || s_canvas == NULL) {
        heap_caps_free(s_images);
        heap_caps_free(s_canvas);
        s_images = NULL;
        s_canvas = NULL;
        return ESP_ERR_NO_MEM;
    }
    for (unsigned i = 0; i < MONITOR_PREVIEW_SLOTS; ++i) {
        s_slots[i].pixels = s_images + i * IMAGE_BYTES;
    }
    s_status.initialized = true;
    return ESP_OK;
}

esp_err_t monitor_preview_submit(const monitor_camera_frame_t *frame)
{
    if (frame == NULL || frame->data == NULL ||
        frame->width != MONITOR_CAMERA_WIDTH || frame->height != MONITOR_CAMERA_HEIGHT ||
        frame->length != MONITOR_CAMERA_FRAME_BYTES || frame->format != MONITOR_CAMERA_RGB565 ||
        frame->sequence == 0 || frame->capture_started_us <= 0) {
        return ESP_ERR_INVALID_ARG;
    }
    const int64_t started_us = esp_timer_get_time();
    if (frame->capture_started_us > started_us ||
        started_us - frame->capture_started_us > MONITOR_PREVIEW_MAX_AGE_US) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    preview_slot_t *slot = NULL;
    portENTER_CRITICAL(&s_lock);
    if (!s_status.initialized) {
        portEXIT_CRITICAL(&s_lock);
        return ESP_ERR_INVALID_STATE;
    }
    const bool same_boot = s_has_source &&
                          memcmp(s_boot_id, frame->boot_id, sizeof(s_boot_id)) == 0;
    if (same_boot && (frame->sequence <= s_sequence || frame->capture_started_us <= s_source_us)) {
        portEXIT_CRITICAL(&s_lock);
        return ESP_ERR_INVALID_RESPONSE;
    }
    for (unsigned i = 0; i < MONITOR_PREVIEW_SLOTS; ++i) {
        if (s_slots[i].state == SLOT_FREE) {
            slot = &s_slots[i];
            slot->state = SLOT_WRITING;
            break;
        }
    }
    portEXIT_CRITICAL(&s_lock);
    if (slot == NULL) return ESP_ERR_INVALID_STATE;

    /* The stock sensor and LCD have different handedness. Reflect preview Y
     * alongside the LCD axis swap; rotate the complete assembly, not its short
     * camera ribbon. Source coordinates/bytes stay native for later calibration.
     */
    for (unsigned y = 0; y < MONITOR_PREVIEW_HEIGHT; ++y) {
        const unsigned source_y = MONITOR_PREVIEW_HEIGHT - 1 - y;
        const size_t source_row = ((2 * source_y + 1) * frame->height /
                                   (2 * MONITOR_PREVIEW_HEIGHT)) * frame->width;
        for (unsigned x = 0; x < MONITOR_PREVIEW_WIDTH; ++x) {
            const size_t source = 2 * (source_row + (2 * x + 1) * frame->width /
                                      (2 * MONITOR_PREVIEW_WIDTH));
            const size_t destination = 2 * (y * MONITOR_PREVIEW_WIDTH + x);
            slot->pixels[destination] = frame->data[source];
            slot->pixels[destination + 1] = frame->data[source + 1];
        }
    }
    slot->sequence = frame->sequence;
    slot->source_us = frame->capture_started_us;
    memcpy(slot->boot_id, frame->boot_id, sizeof(slot->boot_id));
    const int64_t elapsed_us = esp_timer_get_time() - started_us;
    portENTER_CRITICAL(&s_lock);
    for (unsigned i = 0; i < MONITOR_PREVIEW_SLOTS; ++i) {
        if (s_slots[i].state == SLOT_PENDING) {
            s_slots[i].state = SLOT_FREE;
            ++s_status.replaced;
        }
    }
    slot->state = SLOT_PENDING;
    s_has_source = true;
    s_sequence = frame->sequence;
    s_source_us = frame->capture_started_us;
    memcpy(s_boot_id, frame->boot_id, sizeof(s_boot_id));
    ++s_status.submitted;
    s_status.capture_error = ESP_OK;
    if (elapsed_us > s_status.max_submit_us) s_status.max_submit_us = elapsed_us;
    portEXIT_CRITICAL(&s_lock);
    return ESP_OK;
}

esp_err_t monitor_preview_capture_failed(esp_err_t error)
{
    if (error == ESP_OK) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&s_lock);
    if (!s_status.initialized) {
        portEXIT_CRITICAL(&s_lock);
        return ESP_ERR_INVALID_STATE;
    }
    s_status.capture_error = error;
    for (unsigned i = 0; i < MONITOR_PREVIEW_SLOTS; ++i) {
        if (s_slots[i].state == SLOT_PENDING) s_slots[i].state = SLOT_FREE;
    }
    portEXIT_CRITICAL(&s_lock);
    return ESP_OK;
}

static void compose(preview_slot_t *slot, monitor_preview_state_t state, int64_t now_us)
{
    memset(s_canvas, 0, CANVAS_BYTES);
    preview_text(s_canvas, 114, 17, "UP", 0xffff);
    const char *title = "CAMERA PREVIEW";
    if (state == MONITOR_PREVIEW_WAITING) title = "WAITING";
    if (state == MONITOR_PREVIEW_STALE) title = "STALE";
    if (state == MONITOR_PREVIEW_CAPTURE_ERROR) title = "CAMERA ERROR";
    preview_text(s_canvas, (240 - (unsigned)strlen(title) * 6) / 2, 35, title,
                 state == MONITOR_PREVIEW_LIVE ? 0xffff : 0xf800);
    if (slot != NULL && state == MONITOR_PREVIEW_LIVE) {
        for (unsigned y = 0; y < MONITOR_PREVIEW_HEIGHT; ++y) {
            memcpy(s_canvas + 2 * ((y + MONITOR_PREVIEW_IMAGE_Y) * 240 + MONITOR_PREVIEW_IMAGE_X),
                   slot->pixels + 2 * y * MONITOR_PREVIEW_WIDTH, MONITOR_PREVIEW_WIDTH * 2);
        }
    }
    preview_text(s_canvas, 11, 116, "L", 0xffff);
    preview_text(s_canvas, 224, 116, "R", 0xffff);
    if (state == MONITOR_PREVIEW_LIVE) {
        char text[40];
        snprintf(text, sizeof(text), "FRAME %" PRIu64, slot->sequence);
        preview_text(s_canvas, (240 - (unsigned)strlen(text) * 6) / 2, 198, text, 0xffff);
        snprintf(text, sizeof(text), "AGE AT DRAW %" PRId64 " MS", (now_us - slot->source_us) / 1000);
        preview_text(s_canvas, (240 - (unsigned)strlen(text) * 6) / 2, 210, text, 0xffff);
    } else {
        preview_text(s_canvas, 63, 116, "NO CURRENT IMAGE", 0xf800);
    }
    /* Labels distinguish color order from a reflected display's bar positions. */
    preview_text(s_canvas, 97, 219, "R", 0xffff);
    preview_text(s_canvas, 117, 219, "G", 0xffff);
    preview_text(s_canvas, 137, 219, "B", 0xffff);
    preview_fill(s_canvas, 90, 229, 20, 3, 0xf800);
    preview_fill(s_canvas, 110, 229, 20, 3, 0x07e0);
    preview_fill(s_canvas, 130, 229, 20, 3, 0x001f);
}

void monitor_preview_step(void)
{
    portENTER_CRITICAL(&s_lock);
    const bool initialized = s_status.initialized;
    portEXIT_CRITICAL(&s_lock);
    if (!initialized) return;
    int64_t now_us = esp_timer_get_time();
    if (s_transfer) {
        if (!preview_lcd_complete()) {
            if (!s_timeout_reported && now_us - s_draw_started_us >= DRAW_TIMEOUT_US) {
                s_timeout_reported = true;
                portENTER_CRITICAL(&s_lock);
                ++s_status.draw_timeouts;
                s_status.display_error = ESP_ERR_TIMEOUT;
                portEXIT_CRITICAL(&s_lock);
            }
            return; /* The DMA canvas remains borrowed, including after timeout. */
        }
        s_transfer = false;
        portENTER_CRITICAL(&s_lock);
        s_status.transfer_pending = false;
        if (s_drawn_state == MONITOR_PREVIEW_LIVE) {
            ++s_status.rendered;
            s_status.last_rendered_sequence = s_drawn_sequence;
            s_status.last_rendered_capture_us = s_drawn_source_us;
            memcpy(s_status.last_rendered_boot_id, s_drawn_boot_id, sizeof(s_drawn_boot_id));
        }
        s_status.display_error = ESP_OK;
        portEXIT_CRITICAL(&s_lock);
    }
    if (!s_lcd_ready) {
        if (now_us < s_next_init_us) return;
        const esp_err_t error = preview_lcd_init();
        portENTER_CRITICAL(&s_lock);
        s_status.display_error = error;
        s_status.display_ready = error == ESP_OK;
        if (error != ESP_OK) ++s_status.draw_failures;
        portEXIT_CRITICAL(&s_lock);
        if (error != ESP_OK) {
            s_next_init_us = esp_timer_get_time() + INIT_RETRY_US;
            return;
        }
        s_lcd_ready = true;
        now_us = esp_timer_get_time(); /* Native init may have consumed the source's lifetime. */
    }

    preview_slot_t *slot = NULL;
    portENTER_CRITICAL(&s_lock);
    monitor_preview_state_t state = MONITOR_PREVIEW_WAITING;
    if (s_status.capture_error != ESP_OK) state = MONITOR_PREVIEW_CAPTURE_ERROR;
    else if (s_has_source) state = now_us - s_source_us > MONITOR_PREVIEW_MAX_AGE_US ?
                                  MONITOR_PREVIEW_STALE : MONITOR_PREVIEW_LIVE;
    for (unsigned i = 0; i < MONITOR_PREVIEW_SLOTS; ++i) {
        if (s_slots[i].state == SLOT_PENDING) {
            if (state == MONITOR_PREVIEW_LIVE && now_us >= s_slots[i].source_us &&
                now_us - s_slots[i].source_us <= MONITOR_PREVIEW_MAX_AGE_US) {
                slot = &s_slots[i];
                slot->state = SLOT_READING;
            } else {
                s_slots[i].state = SLOT_FREE;
                ++s_status.expired;
            }
        }
    }
    s_status.state = state;
    portEXIT_CRITICAL(&s_lock);
    if (slot == NULL && state == MONITOR_PREVIEW_LIVE) return; /* Keep LCD GRAM; never renew source time. */
    if (slot == NULL && state == s_drawn_state && now_us < s_next_status_us) return;
    compose(slot, state, now_us);
    s_drawn_sequence = slot == NULL ? 0 : slot->sequence;
    s_drawn_source_us = slot == NULL ? 0 : slot->source_us;
    s_drawn_state = state;
    if (slot != NULL) {
        memcpy(s_drawn_boot_id, slot->boot_id, sizeof(s_drawn_boot_id));
        portENTER_CRITICAL(&s_lock);
        slot->state = SLOT_FREE;
        portEXIT_CRITICAL(&s_lock);
    }
    const esp_err_t error = preview_lcd_begin(s_canvas);
    portENTER_CRITICAL(&s_lock);
    s_status.display_error = error;
    if (error != ESP_OK) ++s_status.draw_failures;
    s_status.transfer_pending = error == ESP_OK;
    portEXIT_CRITICAL(&s_lock);
    if (error == ESP_OK) {
        s_transfer = true;
        s_timeout_reported = false;
        s_draw_started_us = esp_timer_get_time();
    }
    s_next_status_us = now_us + DRAW_TIMEOUT_US;
}

esp_err_t monitor_preview_get_status(monitor_preview_status_t *status)
{
    if (status == NULL) return ESP_ERR_INVALID_ARG;
    portENTER_CRITICAL(&s_lock);
    *status = s_status;
    portEXIT_CRITICAL(&s_lock);
    return ESP_OK;
}
