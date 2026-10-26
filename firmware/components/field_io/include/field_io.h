#ifndef FIELD_IO_H
#define FIELD_IO_H

#include <stdint.h>

#define FIELD_IO_EXCLUSION 1

typedef struct field_io_status {
    uint16_t raw_pins;
    int link_ok;
    uint16_t output_polarity;
    uint16_t input_polarity;
} field_io_status_t;

#define FIELD_BIT_MIXER 0
#define FIELD_BIT_CONVEYOR 1
#define FIELD_BIT_DOOR_OPEN 2
#define FIELD_BIT_DOOR_CLOSE 3
#define FIELD_BIT_PLUNGER_EXTEND 4
#define FIELD_BIT_PLUNGER_RETRACT 5
#define FIELD_BIT_CUTTER_EXTEND 6
#define FIELD_BIT_FLATTENER 8

#define FIELD_REQ(bit) ((uint16_t)(1u << (bit)))

int field_io_apply(const field_io_status_t *status, uint16_t requests, uint16_t *image);
int field_io_stop_requested(const field_io_status_t *status);
int field_io_door_closed(const field_io_status_t *status);
int field_io_safety_run(const field_io_status_t *status);
int field_io_air_ok(const field_io_status_t *status);
int field_io_drives_healthy(const field_io_status_t *status);
int field_io_start_pressed(const field_io_status_t *status);
int field_io_reset_pressed(const field_io_status_t *status);

#endif
