idf_component_register(
    SRCS "machine.c"
    INCLUDE_DIRS "include"
    REQUIRES field_io config_store calibration
)
