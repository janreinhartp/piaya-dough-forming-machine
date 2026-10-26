#ifndef ALARM_H
#define ALARM_H

typedef enum alarm_code {
    ALARM_NONE = 0,
    ALARM_ESTOP_ACTIVE,
    ALARM_AIR_LOW,
    ALARM_DOOR_OPEN_DURING_MIX,
    ALARM_DOOR_CLOSE_TIMEOUT,
    ALARM_I2C_IO_FAILURE,
    ALARM_DRIVE_FAULT,
    ALARM_CONFIG_INVALID,
    ALARM_NVS_CORRUPT,
    ALARM_NVS_SAVE_FAIL,
    ALARM_SETTINGS_REJECTED,
    ALARM_OUTPUT_EXCLUSION,
    ALARM_UNCOMMISSIONED
} alarm_code_t;

const char *alarm_message(alarm_code_t code);
const char *alarm_recovery(alarm_code_t code);

#endif
