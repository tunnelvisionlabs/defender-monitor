#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "monitor_camera.h"

#define MONITOR_PREVIEW_WIDTH 192U
#define MONITOR_PREVIEW_HEIGHT 144U
#define MONITOR_PREVIEW_SCREEN_SIZE 240U
#define MONITOR_PREVIEW_IMAGE_X 24U
#define MONITOR_PREVIEW_IMAGE_Y 48U
#define MONITOR_PREVIEW_SLOTS 3U
#define MONITOR_PREVIEW_MAX_AGE_US INT64_C(1000000)

typedef enum {
    MONITOR_PREVIEW_WAITING,
    MONITOR_PREVIEW_LIVE,
    MONITOR_PREVIEW_STALE,
    MONITOR_PREVIEW_CAPTURE_ERROR,
} monitor_preview_state_t;

typedef struct {
    bool initialized;
    bool display_ready;
    bool transfer_pending;
    monitor_preview_state_t state; /* Last selected view; LCD visibility is not acknowledged. */
    uint64_t submitted;
    uint64_t replaced;
    uint64_t expired;
    uint64_t rendered;
    uint64_t draw_failures;
    uint64_t draw_timeouts;
    uint64_t last_rendered_sequence;
    int64_t last_rendered_capture_us;
    uint8_t last_rendered_boot_id[MONITOR_CAMERA_BOOT_ID_BYTES];
    int64_t max_submit_us; /* Largest reservation/copy window, before final publication bookkeeping. */
    esp_err_t capture_error;
    esp_err_t display_error;
} monitor_preview_status_t;

/**
 * Allocate the fixed preview pool once before starting either owner task.
 * Uses three 192x144 RGB565 PSRAM slots and one 240x240 internal DMA canvas.
 * No display, touch, network, storage, or task is initialized here. Failure
 * frees allocations and requires restart; there is no allocation retry loop.
 */
esp_err_t monitor_preview_init(void);

/**
 * Called by the single camera-owner task while its frame lease is valid.
 * Nearest-neighbor scale of the complete 320x240 source preserves aspect ratio
 * and raw MSB-first RGB565 bytes. Preview X is reflected to match the stock
 * camera/LCD orientation with USB down; source coordinates and bytes remain
 * native. No source pointer survives this call. Release the camera lease
 * immediately afterward. There is no wait for LCD/DMA or a
 * queue: the latest pending preview replaces an older one. Only bookkeeping
 * takes a short critical section; image copying occurs outside it.
 *
 * Invalid shape, expired/future source time, or same-session non-increasing
 * sequence/time is rejected without refreshing progress. A new boot token
 * starts a new source session. Frame identity and source time remain unchanged.
 */
esp_err_t monitor_preview_submit(const monitor_camera_frame_t *frame);

/**
 * Same producer task: invalidate queued preview on a failed acquisition/init.
 * A successful submission clears the error. No old image becomes fresh while
 * acquisition is blocked or failing. ESP_OK is an invalid argument.
 */
esp_err_t monitor_preview_capture_failed(esp_err_t error);

/**
 * Called repeatedly by one separate display-owner task, at most 50 ms apart.
 * This owns all LCD calls; SDK initialization/command calls can block and must
 * never run in the capture task. Init failure retries after 5 seconds when SDK
 * cleanup permits. DMA is polled without waiting. Its canvas is never modified
 * until completion, even after the 500 ms timeout. A late completion permits
 * recovery with the latest source. SPI writes cannot verify visible LCD output.
 *
 * Freshness uses source time. Expired/error previews are replaced by a blank
 * status screen. Frozen hardware cannot update that screen; counters and the
 * source-age-at-draw label are diagnostic evidence, not a hardware watchdog.
 */
void monitor_preview_step(void);

/** Thread-safe snapshot from task context; NULL is invalid argument. */
esp_err_t monitor_preview_get_status(monitor_preview_status_t *status);
