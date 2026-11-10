#pragma once
#include "driver/gpio.h"
#include "esp_camera.h"
#include "monitor_camera.h"

typedef struct {
    bool psram;
    bool initialized;
    bool sensor_missing;
    bool timeout;
    bool leased;
    esp_err_t init_error;
    esp_err_t gpio_error;
    esp_err_t deinit_error;
    camera_config_t config;
    gpio_config_t gpio_config;
    uint32_t gpio_levels[49];
    sensor_t sensor;
    camera_fb_t frame;
    uint8_t pixels[MONITOR_CAMERA_FRAME_BYTES];
    int64_t now_us;
    unsigned acquisitions;
    unsigned returns;
    unsigned initializations;
    unsigned cleanups;
} fake_sdk_t;

extern fake_sdk_t fake_sdk;
void fake_sdk_reset(void);
void fake_sdk_prepare_frame(int64_t source_us);
