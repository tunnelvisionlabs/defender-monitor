#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bring-up targets, not measured optical or Home Assistant latency guarantees. */
#define MONITOR_CAMERA_WIDTH 320U
#define MONITOR_CAMERA_HEIGHT 240U
#define MONITOR_CAMERA_FRAME_BYTES (MONITOR_CAMERA_WIDTH * MONITOR_CAMERA_HEIGHT * 2U)
#define MONITOR_CAMERA_MAX_AGE_US INT64_C(1000000)
#define MONITOR_CAMERA_BOOT_ID_BYTES 16U

typedef enum {
    MONITOR_CAMERA_RGB565,
} monitor_camera_format_t;

typedef struct {
    const uint8_t *data;
    size_t length;
    size_t width;
    size_t height;
    monitor_camera_format_t format;
    uint8_t boot_id[MONITOR_CAMERA_BOOT_ID_BYTES];
    uint64_t sequence;
    int64_t capture_started_us;
    int64_t delivered_us;
} monitor_camera_frame_t;

typedef struct {
    bool initialization_attempted;
    bool ready;
    bool frame_outstanding;
    uint8_t boot_id[MONITOR_CAMERA_BOOT_ID_BYTES];
    uint16_t sensor_pid;
    uint64_t attempts;
    uint64_t frames_delivered;
    uint64_t acquisition_failures;
    int64_t last_capture_started_us;
    int64_t last_delivery_us;
    esp_err_t last_error;
    esp_err_t cleanup_error;
} monitor_camera_status_t;

/**
 * Initialize the recorded XIAO ESP32S3 Sense / OV3660 profile once per boot.
 *
 * All calls in this API belong to one serialized owner task (not an ISR).
 * They are not thread-safe. init blocks on SDK camera initialization, configures
 * only the monitor board, and allocates one SDK-owned RGB565 buffer in PSRAM.
 * The LCD and both card selects are set inactive; no SD, display, radio, NVS,
 * household connection, or persistent device identity is initialized.
 *
 * A boot-scoped random correlation token is generated on the first attempt. It
 * is neither a persistent identity nor a security token. Any initialization
 * failure leaves ready false and requires a restart; there is no retry loop.
 * Repeated initialization returns ESP_ERR_INVALID_STATE without resetting age,
 * counters, or the session. A sensor other than OV3660 is rejected.
 */
esp_err_t monitor_camera_init(void);

/**
 * Borrow one frame in caller-owned storage, without copying image bytes.
 *
 * frame must remain at the same address until release. Do not copy a live lease
 * or pass its image pointer onward beyond release. Bytes are read-only RGB565
 * in the sensor/SDK's byte order, unchanged here; verify color/byte order during
 * display bring-up. capture_started_us is the SDK's first-DMA-buffer timestamp
 * in microseconds since boot, not a receive time or UTC. delivered_us uses the
 * same monotonic clock after the SDK returns. sequence increases only for a
 * valid delivered acquisition, including genuinely unchanged pixel content.
 *
 * The pinned SDK waits up to 4 seconds for a buffer per call; scheduling adds
 * latency. There are no internal retries. Invalid dimensions/format/length,
 * non-increasing/future timestamps, or age above MAX_AGE_US return
 * ESP_ERR_INVALID_RESPONSE and immediately return the SDK buffer. A missing
 * buffer returns ESP_ERR_TIMEOUT. No cached frame is substituted.
 *
 * Only one lease may be outstanding. Another acquire returns
 * ESP_ERR_INVALID_STATE without waiting or capturing. On failure frame is
 * cleared, except that an already-outstanding lease at that address is left
 * intact. A slow consumer prevents further delivery; it never renews freshness.
 */
esp_err_t monitor_camera_acquire(monitor_camera_frame_t *frame);

/**
 * Return the active lease once and clear its fields; data becomes invalid.
 * A different address, copied lease, or double release returns
 * ESP_ERR_INVALID_STATE and does not return a buffer. NULL is invalid argument.
 * This only returns ownership; it does not constitute a new acquisition.
 */
esp_err_t monitor_camera_release(monitor_camera_frame_t *frame);

/**
 * Copy current progress from the same owner task; NULL is invalid argument.
 * ready means initialization succeeded, not that a fresh reading exists.
 * Failed acquisition preserves previous timestamps/counters as historical
 * evidence, increments acquisition_failures, and records last_error. Invalid
 * API use does not count as acquisition. cleanup_error reports post-init
 * rejection cleanup separately. This function never polls or refreshes age.
 */
esp_err_t monitor_camera_get_status(monitor_camera_status_t *status);

#ifdef __cplusplus
}
#endif
