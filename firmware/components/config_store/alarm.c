#include "alarm.h"

const char *alarm_message(alarm_code_t code)
{
    switch (code) {
    case ALARM_ESTOP_ACTIVE:
        return "Emergency stop is open.";
    case ALARM_AIR_LOW:
        return "Air pressure is low.";
    case ALARM_DOOR_OPEN_DURING_MIX:
        return "Mixer door opened during mixing.";
    case ALARM_DOOR_CLOSE_TIMEOUT:
        return "Door did not report closed in time.";
    case ALARM_I2C_IO_FAILURE:
        return "Field I/O link failed.";
    case ALARM_DRIVE_FAULT:
        return "A drive reports a fault.";
    case ALARM_CONFIG_INVALID:
        return "The recipe snapshot is not valid.";
    case ALARM_NVS_CORRUPT:
        return "Stored settings were replaced with factory data.";
    case ALARM_NVS_SAVE_FAIL:
        return "The last save did not complete.";
    case ALARM_SETTINGS_REJECTED:
        return "That value is outside its limits.";
    case ALARM_OUTPUT_EXCLUSION:
        return "Both coils of a pair were requested in automatic mode.";
    case ALARM_UNCOMMISSIONED:
        return "This machine has not been commissioned.";
    case ALARM_NONE:
    default:
        return "";
    }
}

const char *alarm_recovery(alarm_code_t code)
{
    switch (code) {
    case ALARM_ESTOP_ACTIVE:
        return "Reset the safety relay with its own button. Then press firmware RESET. Then press START for a new batch.";
    case ALARM_AIR_LOW:
        return "Restore pressure. Press firmware RESET. Check the machine before START.";
    case ALARM_DOOR_OPEN_DURING_MIX:
        return "Close the door. Press firmware RESET. Start a new batch.";
    case ALARM_DOOR_CLOSE_TIMEOUT:
        return "Inspect the door. The timer is not a position proof. Press firmware RESET after the switch reads closed.";
    case ALARM_I2C_IO_FAILURE:
        return "Outputs may still be latched. Remove field power if motion continues. Restore the link. RESET, then a new START.";
    case ALARM_DRIVE_FAULT:
        return "Reset the drive using its own manual. Firmware RESET does not reset the drive.";
    case ALARM_CONFIG_INVALID:
        return "Return to Settings and correct the named field.";
    case ALARM_NVS_CORRUPT:
        return "Commission the machine again. The commissioned flag is clear.";
    case ALARM_NVS_SAVE_FAIL:
        return "The previous saved values remain. Retry the save.";
    case ALARM_SETTINGS_REJECTED:
        return "Read the minimum and maximum on the row.";
    case ALARM_OUTPUT_EXCLUSION:
        return "The outputs were forced off. Press firmware RESET. Start a new batch only after the request that set both coils is corrected.";
    case ALARM_UNCOMMISSIONED:
        return "Engineering must save trial times and set commissioned.";
    case ALARM_NONE:
    default:
        return "";
    }
}
