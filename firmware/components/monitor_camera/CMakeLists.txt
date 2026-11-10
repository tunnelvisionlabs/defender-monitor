idf_component_register(
    SRCS "monitor_camera.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common
    PRIV_REQUIRES esp32-camera esp_driver_gpio esp_psram esp_timer esp_hw_support
)
