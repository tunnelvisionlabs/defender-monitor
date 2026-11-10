#include "preview_text.h"

#include <stddef.h>

/* Small project-owned 5x7 glyphs; each row is a five-bit mask, MSB at left.
 * This diagnostic view needs no GUI toolkit, downloaded font, or text heap.
 */
static const uint8_t digits[][7] = {
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
};
static const uint8_t letters[][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
};

void preview_fill(uint8_t *canvas, unsigned x, unsigned y, unsigned width,
                  unsigned height, uint16_t color)
{
    for (unsigned row = y; row < y + height && row < 240; ++row) {
        for (unsigned col = x; col < x + width && col < 240; ++col) {
            const size_t offset = 2 * (row * 240 + col);
            canvas[offset] = (uint8_t)(color >> 8);
            canvas[offset + 1] = (uint8_t)color;
        }
    }
}

void preview_text(uint8_t *canvas, unsigned x, unsigned y, const char *text,
                  uint16_t color)
{
    for (; *text != '\0' && x + 5 <= 240; ++text, x += 6) {
        const uint8_t *glyph = NULL;
        if (*text >= '0' && *text <= '9') glyph = digits[*text - '0'];
        if (*text >= 'A' && *text <= 'Z') glyph = letters[*text - 'A'];
        if (glyph == NULL) continue;
        for (unsigned row = 0; row < 7; ++row) {
            for (unsigned col = 0; col < 5; ++col) {
                if ((glyph[row] & (1U << (4 - col))) != 0) {
                    preview_fill(canvas, x + col, y + row, 1, 1, color);
                }
            }
        }
    }
}
