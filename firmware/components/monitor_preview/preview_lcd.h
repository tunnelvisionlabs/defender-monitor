#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* Display-owner task only. Native SDK boundary, also used by host fakes.
 * init may retry only after successful cleanup. begin borrows exactly 115200
 * internal DMA bytes until complete() returns true. One transfer at a time;
 * an error from begin means no color transaction was queued. Never abandon or
 * reuse the buffer on timeout. complete is nonblocking and consumes completion.
 */
esp_err_t preview_lcd_init(void);
esp_err_t preview_lcd_begin(const uint8_t *canvas);
bool preview_lcd_complete(void);
