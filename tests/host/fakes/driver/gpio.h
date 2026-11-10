#pragma once
#include <stdint.h>
#include "esp_err.h"
typedef enum { GPIO_NUM_2 = 2, GPIO_NUM_3 = 3, GPIO_NUM_21 = 21 } gpio_num_t;
typedef enum { GPIO_MODE_OUTPUT = 2 } gpio_mode_t;
typedef enum { GPIO_PULLUP_DISABLE } gpio_pullup_t;
typedef enum { GPIO_PULLDOWN_DISABLE } gpio_pulldown_t;
typedef enum { GPIO_INTR_DISABLE } gpio_int_type_t;
typedef struct {
    uint64_t pin_bit_mask;
    gpio_mode_t mode;
    gpio_pullup_t pull_up_en;
    gpio_pulldown_t pull_down_en;
    gpio_int_type_t intr_type;
} gpio_config_t;
esp_err_t gpio_set_level(gpio_num_t pin, uint32_t level);
esp_err_t gpio_config(const gpio_config_t *config);
