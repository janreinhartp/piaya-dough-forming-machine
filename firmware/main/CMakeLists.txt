idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash rgb_lcd_port touch lvgl_port runtime hmi
)
