#ifndef MACHINE_H
#define MACHINE_H

#include <stdint.h>

#include "config_store.h"
#include "field_io.h"

#define MACHINE_CMD_QUEUE 8

typedef enum machine_state {
    M_BOOT = 0,
    M_IDLE,
    M_PRECHECK,
    M_STOPPED,
    M_FAULT,
    M_ESTOP,
    M_MAINT,
    M_DOOR_CLOSE,
    M_MIX_RUN,
    M_XFER_DOOR_PARTIAL,
    M_XFER_DOOR_CENTER_1,
    M_XFER_MIX_1,
    M_XFER_PUSH_1,
    M_XFER_PLUNGER_CENTER_1,
    M_XFER_RETRACT_1,
    M_XFER_DOOR_FULL,
    M_XFER_DOOR_CENTER_2,
    M_XFER_MIX_2,
    M_XFER_PUSH_2,
    M_XFER_PLUNGER_CENTER_2,
    M_XFER_RETRACT_2,
    M_XFER_CONFIRM,
    M_FORM_PLUNGE,
    M_FORM_PLUNGE_CENTER,
    M_FORM_CUT_EXTEND,
    M_FORM_CUT_DWELL,
    M_FORM_CUT_RETRACT,
    M_FORM_CONVEY,
    M_FORM_FLAT_DOWN,
    M_FORM_FLAT_DWELL,
    M_FORM_FLAT_UP,
    M_FORM_COUNT,
    M_FORM_RETRACT,
    M_BATCH_DONE,
    M_STOPPING
} machine_state_t;

typedef enum machine_cmd {
    CMD_START = 1,
    CMD_STOP,
    CMD_RESET,
    CMD_ACK,
    CMD_CLOSE_DOOR,
    CMD_BARREL_READY,
    CMD_MAINT_ENTER,
    CMD_MAINT_EXIT,
    CMD_JOG_PRESS,
    CMD_JOG_RELEASE,
    CMD_CAL_START,
    CMD_CAL_HINT,
    CMD_CAL_CONFIRM,
    CMD_CAL_CANCEL
} machine_cmd_t;

typedef struct machine_command {
    machine_cmd_t cmd;
    uint16_t arg;
    uint32_t arg2;
} machine_command_t;

typedef struct machine {
    config_store_t *store;
    field_io_status_t io;
    machine_cfg_t cfg;
    recipe_blob_t snapshot;
    batch_meta_t batch;
    machine_state_t state;
    machine_state_t door_return;
    machine_state_t after_count;
    int entered;
    int centering;
    uint32_t now_ms;
    uint32_t deadline_ms;
    int deadline_armed;
    uint16_t request;
    uint16_t image;
    uint16_t jog_held;
    uint16_t force_request;
    int force_on;
    alarm_code_t alarm;
    alarm_code_t banner;
    uint32_t pieces;
    uint32_t cycles;
    int interrupted;
    int calibrating;
    uint16_t samples;
    uint16_t sample_target;
    uint32_t hint_ms;
    int hint_ready;
    int command_rejected;
    uint32_t kicks;
    int saw_count;
    int door_stable;
    int door_closed_cached;
    uint32_t door_count_ms;
    uint32_t last_ms;
    machine_command_t queue[MACHINE_CMD_QUEUE];
    int q_head;
    int q_count;
} machine_t;

void machine_boot(machine_t *machine, config_store_t *store, const field_io_status_t *io, uint32_t now_ms);
void machine_set_io(machine_t *machine, const field_io_status_t *io);
int machine_post(machine_t *machine, machine_cmd_t cmd, uint16_t arg, uint32_t arg2);
void machine_force_request(machine_t *machine, uint16_t request);
void machine_poll(machine_t *machine, uint32_t now_ms);

machine_state_t machine_state(const machine_t *machine);
const char *machine_state_name(machine_state_t state);
alarm_code_t machine_alarm(const machine_t *machine);
alarm_code_t machine_banner(const machine_t *machine);
uint16_t machine_image(const machine_t *machine);
uint32_t machine_pieces(const machine_t *machine);
uint32_t machine_cycles(const machine_t *machine);
int machine_interrupted(const machine_t *machine);
int machine_commissioned(const machine_t *machine);
uint16_t machine_samples(const machine_t *machine);
uint32_t machine_hint(const machine_t *machine);
int machine_command_rejected(const machine_t *machine);
uint32_t machine_watchdog_kicks(const machine_t *machine);
int machine_saw_count(const machine_t *machine);

#endif
