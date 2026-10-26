idf_component_register(
    SRCS "runtime.c"
    INCLUDE_DIRS "include"
    REQUIRES diagnostics driver freertos i2c machine pcf8575
)
