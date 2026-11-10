#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define FAKE_CANVAS_BYTES (240U * 240U * 2U)
typedef struct {
    int64_t now_us;
    int64_t init_delay_us;
    esp_err_t init_error;
    esp_err_t draw_error;
    bool borrowed;
    bool completion;
    unsigned init_calls;
    unsigned draws;
    unsigned allocation_calls;
    unsigned fail_allocation;
    size_t live_bytes;
    unsigned critical_depth;
    const uint8_t *canvas;
    uint8_t borrowed_snapshot[FAKE_CANVAS_BYTES];
} fake_preview_t;
extern fake_preview_t fake_preview;
void fake_preview_reset(void);
void fake_preview_verify_dma_ownership(void);
uint16_t fake_preview_pixel(unsigned x, unsigned y);
