#pragma once
#include <stdint.h>

/* Display-owner canvas, 240x240 MSB-first RGB565 bytes. Coordinates and text
 * lengths below are bounded by the fixed preview layout; pixels are clipped.
 */
void preview_fill(uint8_t *canvas, unsigned x, unsigned y, unsigned width,
                  unsigned height, uint16_t color);
void preview_text(uint8_t *canvas, unsigned x, unsigned y, const char *text,
                  uint16_t color);
