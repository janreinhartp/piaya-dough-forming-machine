#include "field_io.h"

enum {
    BIT_MIXER = 0,
    BIT_CONVEYOR = 1,
    BIT_DOOR_OPEN = 2,
    BIT_DOOR_CLOSE = 3,
    BIT_PLUNGER_EXTEND = 4,
    BIT_PLUNGER_RETRACT = 5,
    BIT_CUTTER_EXTEND = 6,
    BIT_FLATTENER = 8,
    BIT_DOOR_CLOSED = 9,
    BIT_SAFETY = 10,
    BIT_AIR = 11,
    BIT_START = 12,
    BIT_STOP = 13,
    BIT_RESET = 14,
    BIT_DRIVE = 15
};

static int pin_is_low(const field_io_status_t *status, int bit)
{
    return (status->raw_pins & (uint16_t)(1u << bit)) == 0;
}

int field_io_safety_run(const field_io_status_t *status)
{
    return pin_is_low(status, BIT_SAFETY);
}

int field_io_air_ok(const field_io_status_t *status)
{
    return pin_is_low(status, BIT_AIR);
}

int field_io_drives_healthy(const field_io_status_t *status)
{
    return pin_is_low(status, BIT_DRIVE);
}

static int permissives_ok(const field_io_status_t *status)
{
    return status->link_ok && field_io_safety_run(status) && field_io_air_ok(status) && field_io_drives_healthy(status);
}

static int active_high(const field_io_status_t *status, int bit)
{
    return (status->output_polarity & (uint16_t)(1u << bit)) != 0;
}

static void write_level(uint16_t *image, int bit, int active, int active_high_bit)
{
    uint16_t mask = (uint16_t)(1u << bit);
    int drive_high = active ? active_high_bit : !active_high_bit;

    if (drive_high) {
        *image = (uint16_t)(*image | mask);
    } else {
        *image = (uint16_t)(*image & (uint16_t)~mask);
    }
}

static uint16_t inactive_image(const field_io_status_t *status)
{
    static const int outputs[] = {
        BIT_MIXER, BIT_CONVEYOR, BIT_DOOR_OPEN, BIT_DOOR_CLOSE, BIT_PLUNGER_EXTEND, BIT_PLUNGER_RETRACT, BIT_CUTTER_EXTEND, BIT_FLATTENER
    };
    uint16_t image = 0xFFFF;
    int i;

    for (i = 0; i < 8; i++) {
        write_level(&image, outputs[i], 0, active_high(status, outputs[i]));
    }
    return image;
}

static int masked_pressed(const field_io_status_t *status, int bit)
{
    int high = !pin_is_low(status, bit);
    int invert = (status->input_polarity & (uint16_t)(1u << bit)) != 0;

    if (invert) {
        return high;
    }
    return !high;
}

int field_io_apply(const field_io_status_t *status, uint16_t requests, uint16_t *image)
{
    static const int outputs[] = {
        BIT_MIXER, BIT_CONVEYOR, BIT_DOOR_OPEN, BIT_DOOR_CLOSE, BIT_PLUNGER_EXTEND, BIT_PLUNGER_RETRACT, BIT_CUTTER_EXTEND, BIT_FLATTENER
    };
    int exclusion = 0;
    int i;

    if (status == 0 || image == 0) {
        return -1;
    }

    *image = inactive_image(status);
    if (!permissives_ok(status)) {
        return 0;
    }

    if ((requests & (uint16_t)((1u << BIT_DOOR_OPEN) | (1u << BIT_DOOR_CLOSE))) == (uint16_t)((1u << BIT_DOOR_OPEN) | (1u << BIT_DOOR_CLOSE))) {
        uint16_t pair = (uint16_t)((1u << BIT_DOOR_OPEN) | (1u << BIT_DOOR_CLOSE));
        requests = (uint16_t)(requests & (uint16_t)~pair);
        exclusion = 1;
    }
    if ((requests & (uint16_t)((1u << BIT_PLUNGER_EXTEND) | (1u << BIT_PLUNGER_RETRACT))) == (uint16_t)((1u << BIT_PLUNGER_EXTEND) | (1u << BIT_PLUNGER_RETRACT))) {
        uint16_t pair = (uint16_t)((1u << BIT_PLUNGER_EXTEND) | (1u << BIT_PLUNGER_RETRACT));
        requests = (uint16_t)(requests & (uint16_t)~pair);
        exclusion = 1;
    }

    for (i = 0; i < 8; i++) {
        int bit = outputs[i];
        if ((requests & (uint16_t)(1u << bit)) != 0) {
            write_level(image, bit, 1, active_high(status, bit));
        }
    }

    return exclusion ? FIELD_IO_EXCLUSION : 0;
}

int field_io_stop_requested(const field_io_status_t *status)
{
    return !pin_is_low(status, BIT_STOP);
}

int field_io_door_closed(const field_io_status_t *status)
{
    return pin_is_low(status, BIT_DOOR_CLOSED);
}

int field_io_start_pressed(const field_io_status_t *status)
{
    return masked_pressed(status, BIT_START);
}

int field_io_reset_pressed(const field_io_status_t *status)
{
    return masked_pressed(status, BIT_RESET);
}
