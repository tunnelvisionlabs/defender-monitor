#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake_sdk.h"
#include "monitor_camera.h"

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static monitor_camera_status_t status(void)
{
    monitor_camera_status_t result;
    CHECK(monitor_camera_get_status(&result) == ESP_OK);
    return result;
}

static void start(void)
{
    CHECK(monitor_camera_init() == ESP_OK);
    CHECK(status().ready);
}

static monitor_camera_frame_t take_and_release(void)
{
    monitor_camera_frame_t frame = {0};
    CHECK(monitor_camera_acquire(&frame) == ESP_OK);
    monitor_camera_frame_t metadata = frame;
    CHECK(monitor_camera_release(&frame) == ESP_OK);
    metadata.data = NULL; /* Keep metadata only, never a released image pointer. */
    return metadata;
}

static void rejects_frame(void)
{
    monitor_camera_frame_t frame = {0};
    CHECK(monitor_camera_acquire(&frame) == ESP_ERR_INVALID_RESPONSE);
    CHECK(frame.data == NULL && frame.length == 0 && frame.sequence == 0);
    CHECK(!fake_sdk.leased && fake_sdk.returns == 1);
    CHECK(status().frames_delivered == 0 && status().acquisition_failures == 1);
    CHECK(status().last_capture_started_us == 0 && !status().frame_outstanding);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    const char *scenario = argv[1];
    fake_sdk_reset();

    if (strcmp(scenario, "uninitialized_has_no_frame") == 0) {
        monitor_camera_frame_t frame = {0};
        CHECK(monitor_camera_acquire(&frame) == ESP_ERR_INVALID_STATE);
        CHECK(frame.data == NULL && !status().ready && fake_sdk.acquisitions == 0);
    } else if (strcmp(scenario, "recorded_profile_uses_one_psram_buffer") == 0) {
        start();
        const camera_config_t *c = &fake_sdk.config;
        CHECK(c->fb_count == 1 && c->fb_location == CAMERA_FB_IN_PSRAM);
        CHECK(c->pixel_format == PIXFORMAT_RGB565 && c->frame_size == FRAMESIZE_QVGA);
        CHECK(c->grab_mode == CAMERA_GRAB_WHEN_EMPTY && c->xclk_freq_hz == 20000000);
        CHECK(c->pin_d0 == 15 && c->pin_d1 == 17 && c->pin_d2 == 18 && c->pin_d3 == 16);
        CHECK(c->pin_d4 == 14 && c->pin_d5 == 12 && c->pin_d6 == 11 && c->pin_d7 == 48);
        CHECK(c->pin_xclk == 10 && c->pin_pclk == 13 && c->pin_vsync == 38 && c->pin_href == 47);
        CHECK(c->pin_sccb_sda == 40 && c->pin_sccb_scl == 39);
        CHECK(c->pin_pwdn == -1 && c->pin_reset == -1);
        CHECK(fake_sdk.gpio_levels[2] == 1 && fake_sdk.gpio_levels[3] == 1 && fake_sdk.gpio_levels[21] == 1);
        CHECK(fake_sdk.gpio_config.pin_bit_mask == ((UINT64_C(1) << 2) | (UINT64_C(1) << 3) | (UINT64_C(1) << 21)));
        CHECK(status().sensor_pid == OV3660_PID && !status().frame_outstanding);
    } else if (strcmp(scenario, "missing_psram_is_not_ready") == 0) {
        fake_sdk.psram = false;
        CHECK(monitor_camera_init() == ESP_ERR_NO_MEM);
        CHECK(!status().ready && status().initialization_attempted && fake_sdk.initializations == 0);
    } else if (strcmp(scenario, "sdk_initialization_failure_is_not_ready") == 0) {
        fake_sdk.init_error = ESP_ERR_NO_MEM;
        CHECK(monitor_camera_init() == ESP_ERR_NO_MEM);
        monitor_camera_frame_t frame = {0};
        CHECK(monitor_camera_acquire(&frame) == ESP_ERR_INVALID_STATE);
        CHECK(!status().ready && status().last_error == ESP_ERR_NO_MEM && frame.data == NULL);
    } else if (strcmp(scenario, "gpio_failure_stops_initialization") == 0) {
        fake_sdk.gpio_error = ESP_FAIL;
        CHECK(monitor_camera_init() == ESP_FAIL);
        CHECK(!status().ready && fake_sdk.initializations == 0);
    } else if (strcmp(scenario, "wrong_sensor_is_rejected") == 0 ||
               strcmp(scenario, "missing_sensor_is_rejected") == 0 ||
               strcmp(scenario, "cleanup_failure_keeps_original_error") == 0) {
        fake_sdk.sensor.id.PID = OV2640_PID;
        fake_sdk.sensor_missing = strcmp(scenario, "missing_sensor_is_rejected") == 0;
        if (strcmp(scenario, "cleanup_failure_keeps_original_error") == 0) {
            fake_sdk.deinit_error = ESP_FAIL;
        }
        CHECK(monitor_camera_init() == ESP_ERR_NOT_SUPPORTED);
        CHECK(!status().ready && fake_sdk.cleanups == 1);
        CHECK(status().cleanup_error == fake_sdk.deinit_error);
        CHECK(monitor_camera_init() == ESP_ERR_INVALID_STATE);
    } else if (strcmp(scenario, "reinitialization_preserves_session_and_progress") == 0) {
        start();
        const monitor_camera_frame_t first = take_and_release();
        CHECK(monitor_camera_init() == ESP_ERR_INVALID_STATE);
        const monitor_camera_status_t after = status();
        CHECK(after.frames_delivered == 1 && fake_sdk.initializations == 1);
        CHECK(memcmp(first.boot_id, after.boot_id, sizeof(first.boot_id)) == 0);
    } else if (strcmp(scenario, "identical_scene_with_new_timestamp_is_new_acquisition") == 0) {
        start();
        monitor_camera_frame_t frame = {0};
        CHECK(monitor_camera_acquire(&frame) == ESP_OK);
        const uint8_t first_pixel = frame.data[0];
        const uint64_t first_sequence = frame.sequence;
        const int64_t first_source = frame.capture_started_us;
        CHECK(monitor_camera_release(&frame) == ESP_OK);
        fake_sdk.now_us += 10000;
        fake_sdk_prepare_frame(fake_sdk.now_us - 1000);
        CHECK(monitor_camera_acquire(&frame) == ESP_OK);
        CHECK(frame.data[0] == first_pixel && frame.sequence == first_sequence + 1);
        CHECK(frame.capture_started_us > first_source && frame.data == fake_sdk.pixels);
        CHECK(monitor_camera_release(&frame) == ESP_OK && frame.data == NULL);
    } else if (strcmp(scenario, "timeout_preserves_historical_progress") == 0) {
        start();
        const monitor_camera_frame_t old = take_and_release();
        fake_sdk.timeout = true;
        fake_sdk.now_us += INT64_C(5000000);
        monitor_camera_frame_t frame = {.length = 42, .sequence = 99};
        CHECK(monitor_camera_acquire(&frame) == ESP_ERR_TIMEOUT);
        const monitor_camera_status_t after = status();
        CHECK(frame.data == NULL && frame.length == 0 && frame.sequence == 0);
        CHECK(after.frames_delivered == 1 && after.acquisition_failures == 1);
        CHECK(after.last_capture_started_us == old.capture_started_us && after.last_delivery_us == old.delivered_us);
        CHECK(after.last_error == ESP_ERR_TIMEOUT && after.ready);
    } else if (strcmp(scenario, "held_frame_prevents_new_acquisition") == 0 ||
               strcmp(scenario, "acquiring_into_live_lease_preserves_it") == 0) {
        start();
        monitor_camera_frame_t held = {0}, other = {0};
        CHECK(monitor_camera_acquire(&held) == ESP_OK);
        const monitor_camera_status_t before = status();
        fake_sdk.now_us += INT64_C(5000000);
        monitor_camera_frame_t *destination = strcmp(scenario, "acquiring_into_live_lease_preserves_it") == 0 ? &held : &other;
        CHECK(monitor_camera_acquire(destination) == ESP_ERR_INVALID_STATE);
        CHECK(held.data == fake_sdk.pixels && held.data[0] == 0x5a);
        CHECK(fake_sdk.acquisitions == 1 && fake_sdk.returns == 0);
        CHECK(status().last_capture_started_us == before.last_capture_started_us && status().frames_delivered == 1);
        CHECK(monitor_camera_release(&held) == ESP_OK);
    } else if (strcmp(scenario, "copied_lease_cannot_release_buffer") == 0) {
        start();
        monitor_camera_frame_t held = {0};
        CHECK(monitor_camera_acquire(&held) == ESP_OK);
        monitor_camera_frame_t copied = held;
        CHECK(monitor_camera_release(&copied) == ESP_ERR_INVALID_STATE);
        CHECK(fake_sdk.leased && fake_sdk.returns == 0 && held.data[0] == 0x5a);
        CHECK(monitor_camera_release(&held) == ESP_OK && held.data == NULL && held.length == 0);
    } else if (strcmp(scenario, "double_release_does_not_return_buffer_again") == 0) {
        start();
        monitor_camera_frame_t frame = {0};
        CHECK(monitor_camera_acquire(&frame) == ESP_OK);
        CHECK(monitor_camera_release(&frame) == ESP_OK);
        CHECK(monitor_camera_release(&frame) == ESP_ERR_INVALID_STATE && fake_sdk.returns == 1);
    } else if (strcmp(scenario, "source_timestamp_is_not_receive_time") == 0) {
        start();
        fake_sdk_prepare_frame(fake_sdk.now_us - 10000);
        const monitor_camera_frame_t frame = take_and_release();
        CHECK(frame.capture_started_us == fake_sdk.now_us - 10000 && frame.delivered_us == fake_sdk.now_us);
    } else if (strcmp(scenario, "stale_frame_is_returned_without_delivery") == 0) {
        start();
        fake_sdk_prepare_frame(fake_sdk.now_us - MONITOR_CAMERA_MAX_AGE_US - 1);
        rejects_frame();
    } else if (strcmp(scenario, "duplicate_timestamp_is_not_new_acquisition") == 0 ||
               strcmp(scenario, "backwards_timestamp_is_not_new_acquisition") == 0) {
        start();
        const monitor_camera_frame_t old = take_and_release();
        fake_sdk.now_us += 1000;
        fake_sdk_prepare_frame(old.capture_started_us - (strcmp(scenario, "backwards_timestamp_is_not_new_acquisition") == 0 ? 1 : 0));
        monitor_camera_frame_t frame = {0};
        CHECK(monitor_camera_acquire(&frame) == ESP_ERR_INVALID_RESPONSE);
        CHECK(frame.data == NULL && status().frames_delivered == 1 && status().acquisition_failures == 1);
        CHECK(status().last_capture_started_us == old.capture_started_us && fake_sdk.returns == 2);
    } else if (strcmp(scenario, "future_timestamp_is_rejected") == 0) {
        start();
        fake_sdk_prepare_frame(fake_sdk.now_us + 1);
        rejects_frame();
    } else if (strcmp(scenario, "invalid_timeval_is_rejected") == 0) {
        start();
        fake_sdk.frame.timestamp.tv_usec = 1000000;
        rejects_frame();
    } else if (strcmp(scenario, "malformed_frame_is_returned_without_delivery") == 0) {
        start();
        fake_sdk.frame.len -= 1;
        rejects_frame();
    } else if (strcmp(scenario, "unexpected_pixel_format_is_rejected") == 0) {
        start();
        fake_sdk.frame.format = PIXFORMAT_YUV422;
        rejects_frame();
    } else if (strcmp(scenario, "missing_image_bytes_are_rejected") == 0) {
        start();
        fake_sdk.frame.buf = NULL;
        rejects_frame();
    } else if (strcmp(scenario, "zero_capture_timestamp_is_rejected") == 0) {
        start();
        fake_sdk.now_us = 1000;
        fake_sdk_prepare_frame(0);
        rejects_frame();
    } else if (strcmp(scenario, "bad_frame_does_not_prevent_fresh_recovery") == 0) {
        start();
        fake_sdk.frame.width = 0;
        rejects_frame();
        fake_sdk.now_us += 1000;
        fake_sdk_prepare_frame(fake_sdk.now_us - 100);
        const monitor_camera_frame_t fresh = take_and_release();
        CHECK(fresh.sequence == 1 && status().acquisition_failures == 1 && status().last_error == ESP_OK);
    } else if (strcmp(scenario, "maximum_allowed_age_is_inclusive") == 0) {
        start();
        fake_sdk_prepare_frame(fake_sdk.now_us - MONITOR_CAMERA_MAX_AGE_US);
        CHECK(take_and_release().sequence == 1);
    } else if (strcmp(scenario, "invalid_arguments_do_not_acquire") == 0) {
        start();
        CHECK(monitor_camera_acquire(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(monitor_camera_release(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(monitor_camera_get_status(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(fake_sdk.acquisitions == 0 && status().frames_delivered == 0);
    } else {
        fprintf(stderr, "Unknown scenario: %s\n", scenario);
        return EXIT_FAILURE;
    }
    CHECK(!fake_sdk.leased);
    printf("PASS %s\n", scenario);
    return EXIT_SUCCESS;
}
