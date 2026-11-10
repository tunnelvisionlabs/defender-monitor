idf_component_register(
    SRCS "monitor_preview.c" "preview_lcd.c" "preview_text.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES monitor_camera esp_common
    PRIV_REQUIRES esp_lcd_gc9a01 esp_lcd esp_driver_spi esp_driver_gpio
                  esp_timer heap freertos
)
