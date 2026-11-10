#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake_preview.h"
#include "monitor_preview.h"

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static uint8_t source[MONITOR_CAMERA_FRAME_BYTES];
static monitor_camera_frame_t frame;

static monitor_preview_status_t status(void)
{
    monitor_preview_status_t result;
    CHECK(monitor_preview_get_status(&result) == ESP_OK);
    return result;
}

static void prepare(uint64_t sequence)
{
    memset(source, 0x5a, sizeof(source));
    frame = (monitor_camera_frame_t) {
        .data = source, .length = sizeof(source),
        .width = 320, .height = 240, .format = MONITOR_CAMERA_RGB565,
        .sequence = sequence, .capture_started_us = fake_preview.now_us - 90000,
        .delivered_us = fake_preview.now_us,
    };
    memset(frame.boot_id, 0x32, sizeof(frame.boot_id));
}

static void start(void)
{
    CHECK(monitor_preview_init() == ESP_OK);
    CHECK(status().initialized && !status().display_ready);
    CHECK(fake_preview.live_bytes == 281088 && fake_preview.allocation_calls == 2);
}

static void submit(void)
{
    CHECK(monitor_preview_submit(&frame) == ESP_OK);
}

static void complete(void)
{
    fake_preview.completion = true;
    monitor_preview_step();
}

static void source_pixel(unsigned x, unsigned y, uint16_t color)
{
    const size_t offset = 2 * (y * 320 + x);
    source[offset] = (uint8_t)(color >> 8);
    source[offset + 1] = (uint8_t)color;
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    const char *scenario = argv[1];
    fake_preview_reset();
    prepare(1);
    if (strcmp(scenario, "uninitialized_does_not_draw_or_borrow") == 0) {
        CHECK(monitor_preview_submit(&frame) == ESP_ERR_INVALID_STATE);
        monitor_preview_step();
        CHECK(fake_preview.draws == 0 && fake_preview.allocation_calls == 0);
    } else if (strcmp(scenario, "allocation_failure_releases_fixed_pool") == 0 ||
               strcmp(scenario, "canvas_allocation_failure_releases_fixed_pool") == 0) {
        fake_preview.fail_allocation = scenario[0] == 'a' ? 1 : 2;
        CHECK(monitor_preview_init() == ESP_ERR_NO_MEM);
        CHECK(fake_preview.live_bytes == 0 && !status().initialized);
        CHECK(monitor_preview_init() == ESP_ERR_INVALID_STATE);
    } else if (strcmp(scenario, "complete_source_corners_and_rgb_bytes_survive_scaling") == 0) {
        start();
        source_pixel(0, 0, 0x1234);
        source_pixel(319, 0, 0x5678);
        source_pixel(0, 239, 0x9abc);
        source_pixel(319, 239, 0xdef0);
        submit();
        monitor_preview_step();
        CHECK(fake_preview_pixel(24, 48) == 0x1234);
        CHECK(fake_preview_pixel(215, 48) == 0x5678);
        CHECK(fake_preview_pixel(24, 191) == 0x9abc);
        CHECK(fake_preview_pixel(215, 191) == 0xdef0);
        CHECK(fake_preview_pixel(91, 223) == 0xf800);
        CHECK(fake_preview_pixel(111, 223) == 0x07e0);
        CHECK(fake_preview_pixel(131, 223) == 0x001f);
        CHECK(status().rendered == 0); /* Submission is not completed output. */
        complete();
        CHECK(status().rendered == 1 && status().last_rendered_sequence == 1);
        CHECK(status().last_rendered_capture_us == frame.capture_started_us);
        CHECK(memcmp(status().last_rendered_boot_id, frame.boot_id, sizeof(frame.boot_id)) == 0);
    } else if (strcmp(scenario, "camera_bytes_can_be_reused_immediately_after_submit") == 0) {
        start();
        submit();
        memset(source, 0xdd, sizeof(source));
        monitor_preview_step();
        CHECK(fake_preview_pixel(100, 100) == 0x5a5a);
    } else if (strcmp(scenario, "slow_display_keeps_only_latest_without_holding_capture") == 0) {
        start();
        submit();
        monitor_preview_step();
        for (uint64_t i = 2; i <= 201; ++i) {
            fake_preview.now_us += 100000;
            prepare(i);
            source_pixel(0, 0, (uint16_t)i);
            submit();
            monitor_preview_step();
            fake_preview_verify_dma_ownership();
        }
        CHECK(status().submitted == 201 && status().replaced == 199);
        CHECK(status().draw_timeouts == 1 && status().transfer_pending);
        CHECK(fake_preview.allocation_calls == 2 && fake_preview.draws == 1);
        complete();
        CHECK(fake_preview_pixel(24, 48) == 201);
        CHECK(fake_preview.draws == 2);
        complete();
        CHECK(status().last_rendered_sequence == 201 && status().display_error == ESP_OK);
    } else if (strcmp(scenario, "source_expiry_blanks_image_without_new_capture") == 0) {
        start();
        submit();
        monitor_preview_step();
        complete();
        const int64_t source_us = status().last_rendered_capture_us;
        fake_preview.now_us = source_us + MONITOR_PREVIEW_MAX_AGE_US + 1;
        monitor_preview_step();
        CHECK(status().state == MONITOR_PREVIEW_STALE);
        CHECK(fake_preview_pixel(100, 100) == 0);
        CHECK(status().last_rendered_capture_us == source_us && status().rendered == 1);
    } else if (strcmp(scenario, "queued_image_expires_before_display_is_available") == 0) {
        start();
        submit();
        fake_preview.now_us += 2000000;
        monitor_preview_step();
        CHECK(status().expired == 1 && status().state == MONITOR_PREVIEW_STALE);
        CHECK(fake_preview_pixel(100, 100) == 0 && status().rendered == 0);
    } else if (strcmp(scenario, "native_initialization_delay_does_not_make_old_image_current") == 0) {
        start();
        submit();
        fake_preview.init_delay_us = 2000000;
        monitor_preview_step();
        CHECK(status().expired == 1 && status().state == MONITOR_PREVIEW_STALE);
        CHECK(fake_preview_pixel(100, 100) == 0);
    } else if (strcmp(scenario, "capture_failure_invalidates_pending_image_and_recovers") == 0) {
        start();
        submit();
        CHECK(monitor_preview_capture_failed(ESP_ERR_TIMEOUT) == ESP_OK);
        monitor_preview_step();
        CHECK(status().state == MONITOR_PREVIEW_CAPTURE_ERROR);
        CHECK(fake_preview_pixel(100, 100) == 0 && status().rendered == 0);
        complete();
        fake_preview.now_us += 100000;
        prepare(2);
        submit();
        monitor_preview_step();
        CHECK(status().state == MONITOR_PREVIEW_LIVE && status().capture_error == ESP_OK);
        complete();
        CHECK(status().last_rendered_sequence == 2);
    } else if (strcmp(scenario, "failure_during_dma_preserves_canvas_then_blanks") == 0) {
        start();
        submit();
        monitor_preview_step();
        CHECK(monitor_preview_capture_failed(ESP_FAIL) == ESP_OK);
        monitor_preview_step();
        fake_preview_verify_dma_ownership();
        CHECK(fake_preview.draws == 1);
        complete();
        CHECK(status().state == MONITOR_PREVIEW_CAPTURE_ERROR && fake_preview.draws == 2);
        CHECK(fake_preview_pixel(100, 100) == 0);
    } else if (strcmp(scenario, "draw_error_does_not_count_as_render_and_next_source_recovers") == 0) {
        start();
        submit();
        fake_preview.draw_error = ESP_FAIL;
        monitor_preview_step();
        CHECK(status().draw_failures == 1 && status().rendered == 0 && !fake_preview.borrowed);
        fake_preview.draw_error = ESP_OK;
        fake_preview.now_us += 100000;
        prepare(2);
        submit();
        monitor_preview_step();
        complete();
        CHECK(status().rendered == 1 && status().last_rendered_sequence == 2);
    } else if (strcmp(scenario, "display_initialization_retries_at_bounded_interval") == 0) {
        start();
        fake_preview.init_error = ESP_FAIL;
        monitor_preview_step();
        CHECK(status().draw_failures == 1 && !status().display_ready);
        fake_preview.now_us += 4999999;
        monitor_preview_step();
        CHECK(fake_preview.init_calls == 1);
        ++fake_preview.now_us;
        fake_preview.init_error = ESP_OK;
        prepare(1);
        submit();
        monitor_preview_step();
        CHECK(status().display_ready && fake_preview.init_calls == 2);
    } else if (strcmp(scenario, "initialization_retry_delay_starts_after_failed_call") == 0) {
        start();
        fake_preview.init_error = ESP_FAIL;
        fake_preview.init_delay_us = 2000000;
        monitor_preview_step();
        fake_preview.now_us += 4999999;
        monitor_preview_step();
        CHECK(fake_preview.init_calls == 1);
        ++fake_preview.now_us;
        monitor_preview_step();
        CHECK(fake_preview.init_calls == 2);
    } else if (strcmp(scenario, "duplicate_or_backwards_identity_does_not_refresh") == 0) {
        start();
        submit();
        const int64_t original_us = frame.capture_started_us;
        fake_preview.now_us += 100000;
        frame.delivered_us = fake_preview.now_us;
        CHECK(monitor_preview_submit(&frame) == ESP_ERR_INVALID_RESPONSE);
        frame.sequence = 2;
        CHECK(monitor_preview_submit(&frame) == ESP_ERR_INVALID_RESPONSE);
        frame.capture_started_us = fake_preview.now_us;
        frame.sequence = 1;
        CHECK(monitor_preview_submit(&frame) == ESP_ERR_INVALID_RESPONSE);
        CHECK(status().submitted == 1);
        monitor_preview_step();
        complete();
        CHECK(status().last_rendered_capture_us == original_us);
    } else if (strcmp(scenario, "new_session_preserves_new_source_identity") == 0) {
        start();
        submit();
        monitor_preview_step();
        complete();
        fake_preview.now_us += 100000;
        prepare(1);
        memset(frame.boot_id, 0x77, sizeof(frame.boot_id));
        submit();
        monitor_preview_step();
        complete();
        CHECK(status().rendered == 2 && status().last_rendered_sequence == 1);
        CHECK(status().last_rendered_boot_id[0] == 0x77);
    } else if (strcmp(scenario, "same_scene_new_source_is_rendered_with_new_identity") == 0) {
        start();
        submit();
        monitor_preview_step();
        complete();
        fake_preview.now_us += 100000;
        prepare(2);
        submit();
        monitor_preview_step();
        CHECK(fake_preview_pixel(100, 100) == 0x5a5a);
        complete();
        CHECK(status().last_rendered_sequence == 2 && status().rendered == 2);
    } else if (strcmp(scenario, "invalid_or_expired_source_is_not_published") == 0) {
        start();
        CHECK(monitor_preview_submit(NULL) == ESP_ERR_INVALID_ARG);
        frame.length = 1;
        CHECK(monitor_preview_submit(&frame) == ESP_ERR_INVALID_ARG);
        prepare(1);
        frame.capture_started_us = fake_preview.now_us + 1;
        CHECK(monitor_preview_submit(&frame) == ESP_ERR_INVALID_RESPONSE);
        frame.capture_started_us = fake_preview.now_us - MONITOR_PREVIEW_MAX_AGE_US - 1;
        CHECK(monitor_preview_submit(&frame) == ESP_ERR_INVALID_RESPONSE);
        CHECK(status().submitted == 0);
    } else if (strcmp(scenario, "maximum_source_age_is_inclusive") == 0) {
        start();
        frame.capture_started_us = fake_preview.now_us - MONITOR_PREVIEW_MAX_AGE_US;
        submit();
        monitor_preview_step();
        CHECK(status().state == MONITOR_PREVIEW_LIVE);
    } else if (strcmp(scenario, "status_reads_and_ticks_do_not_renew_source") == 0) {
        start();
        submit();
        monitor_preview_step();
        complete();
        const monitor_preview_status_t before = status();
        fake_preview.now_us += 300000;
        for (unsigned i = 0; i < 5; ++i) monitor_preview_step();
        CHECK(fake_preview.draws == 1 && status().rendered == before.rendered);
        CHECK(status().last_rendered_capture_us == before.last_rendered_capture_us);
    } else if (strcmp(scenario, "invalid_api_calls_preserve_initialized_preview") == 0) {
        start();
        CHECK(monitor_preview_init() == ESP_ERR_INVALID_STATE);
        CHECK(monitor_preview_capture_failed(ESP_OK) == ESP_ERR_INVALID_ARG);
        CHECK(monitor_preview_get_status(NULL) == ESP_ERR_INVALID_ARG);
        CHECK(status().initialized && status().capture_error == ESP_OK);
    } else {
        fprintf(stderr, "Unknown scenario: %s\n", scenario);
        return EXIT_FAILURE;
    }
    fake_preview_verify_dma_ownership();
    CHECK(fake_preview.critical_depth == 0);
    printf("PASS %s\n", scenario);
    return EXIT_SUCCESS;
}
