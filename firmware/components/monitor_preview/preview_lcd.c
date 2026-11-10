#include "preview_lcd.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LCD_HOST SPI2_HOST
#define CANVAS_BYTES (240 * 240 * 2)

static esp_lcd_panel_io_handle_t s_io;
static esp_lcd_panel_handle_t s_panel;
static StaticSemaphore_t s_done_storage;
static SemaphoreHandle_t s_done;
static bool s_ready;
static bool s_cleanup_failed;
static bool s_pending;

static bool transfer_done(esp_lcd_panel_io_handle_t io,
                          esp_lcd_panel_io_event_data_t *event, void *context)
{
    (void)io;
    (void)event;
    (void)context;
    BaseType_t awakened = pdFALSE;
    xSemaphoreGiveFromISR(s_done, &awakened);
    return awakened == pdTRUE;
}

esp_err_t preview_lcd_init(void)
{
    if (s_ready) return ESP_OK;
    if (s_cleanup_failed) return ESP_ERR_INVALID_STATE;
    if (s_done == NULL) s_done = xSemaphoreCreateBinaryStatic(&s_done_storage);
    if (s_done == NULL) return ESP_ERR_NO_MEM;
    /* Preserve both SD selects and the shared active-low user LED. KE stays open. */
    esp_err_t error = gpio_set_level(GPIO_NUM_3, 1);
    if (error == ESP_OK) error = gpio_set_level(GPIO_NUM_21, 1);
    const gpio_config_t selects = {
        .pin_bit_mask = (UINT64_C(1) << GPIO_NUM_3) | (UINT64_C(1) << GPIO_NUM_21),
        .mode = GPIO_MODE_OUTPUT,
    };
    if (error == ESP_OK) error = gpio_config(&selects);
    if (error != ESP_OK) return error;

    const spi_bus_config_t bus = {
        .sclk_io_num = 7,
        .mosi_io_num = 9,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CANVAS_BYTES,
    };
    error = spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO);
    if (error != ESP_OK) return error;
    const esp_lcd_panel_io_spi_config_t io = {
        .cs_gpio_num = 2,
        .dc_gpio_num = 4,
        .spi_mode = 0,
        .pclk_hz = 20000000,
        .trans_queue_depth = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .on_color_trans_done = transfer_done,
    };
    error = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io, &s_io);
    const esp_lcd_panel_dev_config_t panel = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
    };
    if (error == ESP_OK) error = esp_lcd_new_panel_gc9a01(s_io, &panel, &s_panel);
    if (error == ESP_OK) error = esp_lcd_panel_reset(s_panel);
    if (error == ESP_OK) error = esp_lcd_panel_init(s_panel);
    if (error == ESP_OK) error = esp_lcd_panel_invert_color(s_panel, true);
    if (error == ESP_OK) error = esp_lcd_panel_swap_xy(s_panel, false);
    if (error == ESP_OK) error = esp_lcd_panel_mirror(s_panel, false, false);
    if (error == ESP_OK) error = esp_lcd_panel_disp_on_off(s_panel, true);
    if (error == ESP_OK) {
        s_ready = true;
        return ESP_OK;
    }
    /* No color DMA was submitted during init. Retry only after complete cleanup. */
    if (s_panel != NULL && esp_lcd_panel_del(s_panel) != ESP_OK) s_cleanup_failed = true;
    s_panel = NULL;
    if (s_io != NULL && esp_lcd_panel_io_del(s_io) != ESP_OK) s_cleanup_failed = true;
    s_io = NULL;
    if (spi_bus_free(LCD_HOST) != ESP_OK) s_cleanup_failed = true;
    return error;
}

esp_err_t preview_lcd_begin(const uint8_t *canvas)
{
    if (!s_ready || s_pending) return ESP_ERR_INVALID_STATE;
    if (canvas == NULL) return ESP_ERR_INVALID_ARG;
    (void)xSemaphoreTake(s_done, 0);
    /* The whole canvas fits one native color transaction. In the pinned SDK,
     * an error before its successful queue operation leaves no color DMA owned.
     */
    const esp_err_t error = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, 240, 240, canvas);
    s_pending = error == ESP_OK;
    return error;
}

bool preview_lcd_complete(void)
{
    if (!s_pending || xSemaphoreTake(s_done, 0) != pdTRUE) return false;
    s_pending = false;
    return true;
}
