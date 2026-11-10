#include "fake_sdk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

fake_sdk_t fake_sdk;

static void require(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "SDK boundary misuse: %s\n", message);
        abort();
    }
}

void fake_sdk_prepare_frame(int64_t source_us)
{
    require(!fake_sdk.leased, "preparing a buffer still owned by the consumer");
    memset(fake_sdk.pixels, 0x5a, sizeof(fake_sdk.pixels));
    fake_sdk.frame = (camera_fb_t) {
        .buf = fake_sdk.pixels,
        .len = sizeof(fake_sdk.pixels),
        .width = MONITOR_CAMERA_WIDTH,
        .height = MONITOR_CAMERA_HEIGHT,
        .format = PIXFORMAT_RGB565,
    };
    fake_sdk.frame.timestamp.tv_sec = (long)(source_us / INT64_C(1000000));
    fake_sdk.frame.timestamp.tv_usec = (long)(source_us % INT64_C(1000000));
}

void fake_sdk_reset(void)
{
    memset(&fake_sdk, 0, sizeof(fake_sdk));
    fake_sdk.psram = true;
    fake_sdk.sensor.id.PID = OV3660_PID;
    fake_sdk.now_us = INT64_C(2000000);
    fake_sdk_prepare_frame(fake_sdk.now_us - 1000);
}

bool esp_psram_is_initialized(void) { return fake_sdk.psram; }
int64_t esp_timer_get_time(void) { return fake_sdk.now_us; }

void esp_fill_random(void *buffer, size_t length)
{
    uint8_t *bytes = buffer;
    for (size_t i = 0; i < length; ++i) {
        bytes[i] = (uint8_t)(0xa0 + i);
    }
}

esp_err_t gpio_set_level(gpio_num_t pin, uint32_t level)
{
    if (fake_sdk.gpio_error != ESP_OK) {
        return fake_sdk.gpio_error;
    }
    require((unsigned)pin < 49, "GPIO outside the recorded chip range");
    fake_sdk.gpio_levels[pin] = level;
    return ESP_OK;
}

esp_err_t gpio_config(const gpio_config_t *config)
{
    fake_sdk.gpio_config = *config;
    return fake_sdk.gpio_error;
}

esp_err_t esp_camera_init(const camera_config_t *config)
{
    ++fake_sdk.initializations;
    require(!fake_sdk.initialized, "initializing an already initialized SDK");
    fake_sdk.config = *config;
    fake_sdk.initialized = fake_sdk.init_error == ESP_OK;
    return fake_sdk.init_error;
}

sensor_t *esp_camera_sensor_get(void)
{
    return fake_sdk.initialized && !fake_sdk.sensor_missing ? &fake_sdk.sensor : NULL;
}

esp_err_t esp_camera_deinit(void)
{
    ++fake_sdk.cleanups;
    require(!fake_sdk.leased, "deinitializing an outstanding SDK buffer");
    if (fake_sdk.deinit_error == ESP_OK) {
        fake_sdk.initialized = false;
    }
    return fake_sdk.deinit_error;
}

camera_fb_t *esp_camera_fb_get(void)
{
    ++fake_sdk.acquisitions;
    if (!fake_sdk.initialized || fake_sdk.timeout || fake_sdk.leased) {
        return NULL;
    }
    fake_sdk.leased = true;
    return &fake_sdk.frame;
}

void esp_camera_fb_return(camera_fb_t *frame)
{
    require(fake_sdk.leased && frame == &fake_sdk.frame, "returning an unowned buffer");
    ++fake_sdk.returns;
    fake_sdk.leased = false;
    /* Make accidental retention of released image content observable. */
    memset(fake_sdk.pixels, 0xdd, sizeof(fake_sdk.pixels));
}
