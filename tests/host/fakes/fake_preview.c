#include "fake_preview.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "preview_lcd.h"

fake_preview_t fake_preview;
static void *allocations[2];
static size_t sizes[2];

static void require(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "Preview boundary misuse: %s\n", message);
        abort();
    }
}

void fake_preview_reset(void)
{
    memset(&fake_preview, 0, sizeof(fake_preview));
    fake_preview.now_us = INT64_C(2000000);
}

void fake_preview_verify_dma_ownership(void)
{
    if (fake_preview.borrowed) {
        require(memcmp(fake_preview.canvas, fake_preview.borrowed_snapshot,
                       FAKE_CANVAS_BYTES) == 0, "modifying a canvas still owned by DMA");
    }
}

int64_t esp_timer_get_time(void)
{
    fake_preview_verify_dma_ownership();
    return fake_preview.now_us;
}

void *heap_caps_malloc(size_t size, unsigned capabilities)
{
    require(fake_preview.critical_depth == 0, "allocating in a critical section");
    ++fake_preview.allocation_calls;
    if (fake_preview.fail_allocation == fake_preview.allocation_calls) return NULL;
    require(fake_preview.allocation_calls <= 2, "allocating beyond the fixed pool");
    if (size == FAKE_CANVAS_BYTES) {
        require((capabilities & (MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA)) ==
                (MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA), "DMA canvas in unsuitable memory");
    } else {
        require(size == 3 * 192 * 144 * 2 && (capabilities & MALLOC_CAP_SPIRAM) != 0,
                "unexpected image pool size or location");
    }
    void *result = malloc(size);
    require(result != NULL, "host allocation failed");
    allocations[fake_preview.allocation_calls - 1] = result;
    sizes[fake_preview.allocation_calls - 1] = size;
    fake_preview.live_bytes += size;
    return result;
}

void heap_caps_free(void *pointer)
{
    if (pointer == NULL) return;
    for (unsigned i = 0; i < 2; ++i) {
        if (allocations[i] == pointer) {
            fake_preview.live_bytes -= sizes[i];
            free(pointer);
            allocations[i] = NULL;
            return;
        }
    }
    require(false, "freeing an unknown allocation");
}

void fake_enter_critical(portMUX_TYPE *lock)
{
    (void)lock;
    require(fake_preview.critical_depth++ == 0, "unexpected nested critical section");
}

void fake_exit_critical(portMUX_TYPE *lock)
{
    (void)lock;
    require(fake_preview.critical_depth-- == 1, "unbalanced critical section");
}

esp_err_t preview_lcd_init(void)
{
    require(fake_preview.critical_depth == 0, "initializing LCD under the producer lock");
    ++fake_preview.init_calls;
    fake_preview.now_us += fake_preview.init_delay_us;
    return fake_preview.init_error;
}

esp_err_t preview_lcd_begin(const uint8_t *canvas)
{
    require(fake_preview.critical_depth == 0, "calling LCD under the producer lock");
    require(!fake_preview.borrowed, "queueing another draw with an outstanding canvas");
    ++fake_preview.draws;
    if (fake_preview.draw_error != ESP_OK) return fake_preview.draw_error;
    fake_preview.canvas = canvas;
    memcpy(fake_preview.borrowed_snapshot, canvas, FAKE_CANVAS_BYTES);
    fake_preview.borrowed = true;
    return ESP_OK;
}

bool preview_lcd_complete(void)
{
    require(fake_preview.critical_depth == 0, "polling LCD under the producer lock");
    require(fake_preview.borrowed, "polling without an active transfer");
    fake_preview_verify_dma_ownership();
    if (!fake_preview.completion) return false;
    fake_preview.completion = false;
    fake_preview.borrowed = false;
    return true;
}

uint16_t fake_preview_pixel(unsigned x, unsigned y)
{
    require(x < 240 && y < 240 && fake_preview.canvas != NULL, "pixel outside the canvas");
    const size_t offset = 2 * (y * 240 + x);
    return (uint16_t)((fake_preview.canvas[offset] << 8) | fake_preview.canvas[offset + 1]);
}
