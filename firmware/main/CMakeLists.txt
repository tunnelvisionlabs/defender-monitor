idf_component_register(
    SRCS "app_main.c"
    PRIV_REQUIRES monitor_camera monitor_preview esp_app_format esp_system spi_flash esp_psram
                  esp_timer heap freertos
)
