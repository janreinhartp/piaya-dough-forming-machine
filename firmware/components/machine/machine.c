#include "machine.h"

#include "calibration.h"

#include <string.h>

static int is_production(machine_state_t state)
{
    switch (state) {
    case M_MIX_RUN:
    case M_XFER_DOOR_PARTIAL:
    case M_XFER_DOOR_CENTER_1:
    case M_XFER_MIX_1:
    case M_XFER_PUSH_1:
    case M_XFER_PLUNGER_CENTER_1:
    case M_XFER_RETRACT_1:
    case M_XFER_DOOR_FULL:
    case M_XFER_DOOR_CENTER_2:
    case M_XFER_MIX_2:
    case M_XFER_PUSH_2:
    case M_XFER_PLUNGER_CENTER_2:
    case M_XFER_RETRACT_2:
    case M_XFER_CONFIRM:
    case M_FORM_PLUNGE:
    case M_FORM_PLUNGE_CENTER:
    case M_FORM_CUT_EXTEND:
    case M_FORM_CUT_DWELL:
    case M_FORM_CUT_RETRACT:
    case M_FORM_CONVEY:
    case M_FORM_FLAT_DOWN:
    case M_FORM_FLAT_DWELL:
    case M_FORM_FLAT_UP:
    case M_FORM_COUNT:
    case M_FORM_RETRACT:
    case M_DOOR_CLOSE:
        return 1;
    default:
        return 0;
    }
}

static void enter_state(machine_t *machine, machine_state_t state)
{
    machine->state = state;
    machine->entered = 0;
    machine->centering = 0;
    machine->deadline_armed = 0;
}

static void arm(machine_t *machine, uint32_t duration_ms)
{
    machine->deadline_ms = machine->now_ms + duration_ms;
    machine->deadline_armed = 1;
}

static int due(const machine_t *machine)
{
    return machine->deadline_armed && machine->now_ms >= machine->deadline_ms;
}

static void raise_alarm(machine_t *machine, alarm_code_t code)
{
    machine->alarm = code;
    if (machine->store != 0) {
        config_store_push_alarm(machine->store, code, (uint16_t)machine->state);
    }
}

static void motion(machine_t *machine, uint16_t request, uint32_t duration_ms)
{
    machine->request = request;
    arm(machine, duration_ms);
}

static const recipe_blob_t *selected_recipe(const machine_t *machine)
{
    return config_store_recipe(machine->store, machine->cfg.selected_slot);
}

static void sample_door(machine_t *machine)
{
    int raw = field_io_door_closed(&machine->io);
    uint32_t elapsed = 0;

    if (machine->now_ms >= machine->last_ms) {
        elapsed = machine->now_ms - machine->last_ms;
    }
    if (machine->cfg.debounce_ms == 0) {
        machine->door_stable = raw;
        machine->door_closed_cached = raw;
        return;
    }
    if (raw == machine->door_stable) {
        machine->door_count_ms = 0;
    } else {
        machine->door_count_ms += elapsed;
        if (machine->door_count_ms >= machine->cfg.debounce_ms) {
            machine->door_stable = raw;
            machine->door_count_ms = 0;
        }
    }
    machine->door_closed_cached = machine->door_stable;
}

static int fault_cause_clear(const machine_t *machine)
{
    if (!machine->io.link_ok || !field_io_air_ok(&machine->io) || !field_io_drives_healthy(&machine->io)) {
        return 0;
    }
    if (machine->alarm == ALARM_DOOR_OPEN_DURING_MIX || machine->alarm == ALARM_DOOR_CLOSE_TIMEOUT) {
        return machine->door_closed_cached;
    }
    return 1;
}

static int conflicts(uint16_t held, uint16_t incoming)
{
    uint16_t both = (uint16_t)(held | incoming);

    if ((both & FIELD_REQ(FIELD_BIT_DOOR_OPEN)) != 0 && (both & FIELD_REQ(FIELD_BIT_DOOR_CLOSE)) != 0) {
        return 1;
    }
    if ((both & FIELD_REQ(FIELD_BIT_PLUNGER_EXTEND)) != 0 && (both & FIELD_REQ(FIELD_BIT_PLUNGER_RETRACT)) != 0) {
        return 1;
    }
    return 0;
}

static void run_entry(machine_t *machine)
{
    const recipe_blob_t *recipe;
    uint32_t close_ms;

    switch (machine->state) {
    case M_IDLE:
    case M_STOPPED:
    case M_FAULT:
    case M_ESTOP:
    case M_BATCH_DONE:
    case M_XFER_CONFIRM:
        machine->request = 0;
        break;
    case M_MAINT:
        machine->request = machine->jog_held;
        break;
    case M_PRECHECK:
        machine->request = 0;
        recipe = selected_recipe(machine);
        if (recipe != 0) {
            machine->snapshot = *recipe;
        }
        break;
    case M_DOOR_CLOSE:
        machine->request = FIELD_REQ(FIELD_BIT_DOOR_CLOSE);
        close_ms = machine->snapshot.door_close_ms;
        recipe = selected_recipe(machine);
        if (machine->door_return == M_IDLE && recipe != 0) {
            close_ms = recipe->door_close_ms;
        }
        arm(machine, close_ms + machine->cfg.door_close_margin_ms);
        break;
    case M_MIX_RUN:
        machine->interrupted = 1;
        machine->banner = ALARM_NONE;
        motion(machine, FIELD_REQ(FIELD_BIT_MIXER), machine->snapshot.mix_ms);
        if (!machine->door_closed_cached) {
            raise_alarm(machine, ALARM_DOOR_OPEN_DURING_MIX);
            enter_state(machine, M_FAULT);
        }
        break;
    case M_XFER_DOOR_PARTIAL:
        motion(machine, FIELD_REQ(FIELD_BIT_DOOR_OPEN), machine->snapshot.door_partial_ms);
        break;
    case M_XFER_DOOR_CENTER_1:
    case M_XFER_DOOR_CENTER_2:
    case M_XFER_PLUNGER_CENTER_1:
    case M_XFER_PLUNGER_CENTER_2:
    case M_FORM_PLUNGE_CENTER:
        motion(machine, 0, 0);
        break;
    case M_XFER_MIX_1:
        motion(machine, FIELD_REQ(FIELD_BIT_MIXER), machine->snapshot.xfer_mix_1_ms);
        break;
    case M_XFER_MIX_2:
        motion(machine, FIELD_REQ(FIELD_BIT_MIXER), machine->snapshot.xfer_mix_2_ms);
        break;
    case M_XFER_PUSH_1:
        motion(machine, FIELD_REQ(FIELD_BIT_PLUNGER_EXTEND), machine->snapshot.push_1_ms);
        break;
    case M_XFER_PUSH_2:
        motion(machine, FIELD_REQ(FIELD_BIT_PLUNGER_EXTEND), machine->snapshot.push_2_ms);
        break;
    case M_XFER_RETRACT_1:
        motion(machine, FIELD_REQ(FIELD_BIT_PLUNGER_RETRACT), machine->snapshot.retract_1_ms);
        break;
    case M_XFER_RETRACT_2:
        motion(machine, FIELD_REQ(FIELD_BIT_PLUNGER_RETRACT), machine->snapshot.retract_2_ms);
        break;
    case M_XFER_DOOR_FULL:
        motion(machine, FIELD_REQ(FIELD_BIT_DOOR_OPEN), machine->snapshot.door_full_ms);
        break;
    case M_FORM_PLUNGE:
        motion(machine, FIELD_REQ(FIELD_BIT_PLUNGER_EXTEND), machine->snapshot.forming_pulse_ms);
        break;
    case M_FORM_CUT_EXTEND:
        motion(machine, FIELD_REQ(FIELD_BIT_CUTTER_EXTEND), machine->snapshot.cutter_extend_ms);
        break;
    case M_FORM_CUT_DWELL:
        motion(machine, FIELD_REQ(FIELD_BIT_CUTTER_EXTEND), machine->snapshot.cutter_dwell_ms);
        break;
    case M_FORM_CUT_RETRACT:
        motion(machine, 0, machine->snapshot.cutter_retract_ms);
        break;
    case M_FORM_CONVEY:
        motion(machine, FIELD_REQ(FIELD_BIT_CONVEYOR), machine->snapshot.conveyor_ms);
        break;
    case M_FORM_FLAT_DOWN:
        motion(machine, FIELD_REQ(FIELD_BIT_FLATTENER), machine->snapshot.flat_down_ms);
        break;
    case M_FORM_FLAT_DWELL:
        motion(machine, FIELD_REQ(FIELD_BIT_FLATTENER), machine->snapshot.flat_dwell_ms);
        break;
    case M_FORM_FLAT_UP:
        motion(machine, 0, machine->snapshot.flat_up_ms);
        break;
    case M_FORM_COUNT:
        machine->request = 0;
        machine->saw_count = 1;
        machine->cycles += 1;
        machine->pieces += machine->snapshot.nozzle_count;
        if (machine->store == 0 || config_store_checkpoint(machine->store, machine->pieces, machine->cycles, machine->interrupted) != 0) {
            raise_alarm(machine, ALARM_NVS_SAVE_FAIL);
            enter_state(machine, M_FAULT);
            break;
        }
        arm(machine, 0);
        break;
    case M_FORM_RETRACT:
        motion(machine, FIELD_REQ(FIELD_BIT_PLUNGER_RETRACT), machine->snapshot.forming_retract_ms);
        break;
    case M_STOPPING:
        motion(machine, 0, 0);
        break;
    case M_BOOT:
    default:
        machine->request = 0;
        break;
    }
}

static void run_body(machine_t *machine)
{
    switch (machine->state) {
    case M_PRECHECK:
        if (!machine->cfg.commissioned) {
            machine->banner = ALARM_UNCOMMISSIONED;
            enter_state(machine, M_IDLE);
            break;
        }
        if (!config_store_recipe_valid(&machine->snapshot, &machine->cfg)) {
            raise_alarm(machine, ALARM_CONFIG_INVALID);
            enter_state(machine, M_IDLE);
            break;
        }
        if (!machine->door_closed_cached) {
            enter_state(machine, M_IDLE);
            break;
        }
        enter_state(machine, M_MIX_RUN);
        break;
    case M_MIX_RUN:
        machine->request = FIELD_REQ(FIELD_BIT_MIXER);
        if (!machine->door_closed_cached) {
            machine->request = 0;
            raise_alarm(machine, ALARM_DOOR_OPEN_DURING_MIX);
            enter_state(machine, M_FAULT);
            break;
        }
        if (due(machine)) {
            enter_state(machine, M_XFER_DOOR_PARTIAL);
        }
        break;
    case M_XFER_DOOR_PARTIAL:
        machine->request = FIELD_REQ(FIELD_BIT_DOOR_OPEN);
        if (due(machine)) {
            enter_state(machine, M_XFER_DOOR_CENTER_1);
        }
        break;
    case M_XFER_DOOR_CENTER_1:
        machine->request = 0;
        if (due(machine)) {
            enter_state(machine, M_XFER_MIX_1);
        }
        break;
    case M_XFER_MIX_1:
        machine->request = FIELD_REQ(FIELD_BIT_MIXER);
        if (due(machine)) {
            enter_state(machine, M_XFER_PUSH_1);
        }
        break;
    case M_XFER_PUSH_1:
        machine->request = FIELD_REQ(FIELD_BIT_PLUNGER_EXTEND);
        if (due(machine)) {
            enter_state(machine, M_XFER_PLUNGER_CENTER_1);
        }
        break;
    case M_XFER_PLUNGER_CENTER_1:
        machine->request = 0;
        if (due(machine)) {
            if (machine->snapshot.retract_1_ms > 0) {
                enter_state(machine, M_XFER_RETRACT_1);
            } else {
                enter_state(machine, M_XFER_DOOR_FULL);
            }
        }
        break;
    case M_XFER_RETRACT_1:
        if (!machine->centering) {
            machine->request = FIELD_REQ(FIELD_BIT_PLUNGER_RETRACT);
            if (due(machine)) {
                machine->request = 0;
                machine->centering = 1;
                arm(machine, 0);
            }
        } else if (due(machine)) {
            enter_state(machine, M_XFER_DOOR_FULL);
        }
        break;
    case M_XFER_DOOR_FULL:
        machine->request = FIELD_REQ(FIELD_BIT_DOOR_OPEN);
        if (due(machine)) {
            enter_state(machine, M_XFER_DOOR_CENTER_2);
        }
        break;
    case M_XFER_DOOR_CENTER_2:
        machine->request = 0;
        if (due(machine)) {
            enter_state(machine, M_XFER_MIX_2);
        }
        break;
    case M_XFER_MIX_2:
        machine->request = FIELD_REQ(FIELD_BIT_MIXER);
        if (due(machine)) {
            enter_state(machine, M_XFER_PUSH_2);
        }
        break;
    case M_XFER_PUSH_2:
        machine->request = FIELD_REQ(FIELD_BIT_PLUNGER_EXTEND);
        if (due(machine)) {
            enter_state(machine, M_XFER_PLUNGER_CENTER_2);
        }
        break;
    case M_XFER_PLUNGER_CENTER_2:
        machine->request = 0;
        if (due(machine)) {
            if (machine->snapshot.retract_2_ms > 0) {
                enter_state(machine, M_XFER_RETRACT_2);
            } else {
                enter_state(machine, M_XFER_CONFIRM);
            }
        }
        break;
    case M_XFER_RETRACT_2:
        if (!machine->centering) {
            machine->request = FIELD_REQ(FIELD_BIT_PLUNGER_RETRACT);
            if (due(machine)) {
                machine->request = 0;
                machine->centering = 1;
                arm(machine, 0);
            }
        } else if (due(machine)) {
            enter_state(machine, M_XFER_CONFIRM);
        }
        break;
    case M_FORM_PLUNGE:
        machine->request = FIELD_REQ(FIELD_BIT_PLUNGER_EXTEND);
        if (due(machine)) {
            enter_state(machine, M_FORM_PLUNGE_CENTER);
        }
        break;
    case M_FORM_PLUNGE_CENTER:
        machine->request = 0;
        if (due(machine)) {
            enter_state(machine, M_FORM_CUT_EXTEND);
        }
        break;
    case M_FORM_CUT_EXTEND:
        machine->request = FIELD_REQ(FIELD_BIT_CUTTER_EXTEND);
        if (due(machine)) {
            if (machine->snapshot.cutter_dwell_ms > 0) {
                enter_state(machine, M_FORM_CUT_DWELL);
            } else {
                enter_state(machine, M_FORM_CUT_RETRACT);
            }
        }
        break;
    case M_FORM_CUT_DWELL:
        machine->request = FIELD_REQ(FIELD_BIT_CUTTER_EXTEND);
        if (due(machine)) {
            enter_state(machine, M_FORM_CUT_RETRACT);
        }
        break;
    case M_FORM_CUT_RETRACT:
        machine->request = 0;
        if (due(machine)) {
            enter_state(machine, M_FORM_CONVEY);
        }
        break;
    case M_FORM_CONVEY:
        machine->request = FIELD_REQ(FIELD_BIT_CONVEYOR);
        if (due(machine)) {
            enter_state(machine, M_FORM_FLAT_DOWN);
        }
        break;
    case M_FORM_FLAT_DOWN:
        machine->request = FIELD_REQ(FIELD_BIT_FLATTENER);
        if (due(machine)) {
            enter_state(machine, M_FORM_FLAT_DWELL);
        }
        break;
    case M_FORM_FLAT_DWELL:
        machine->request = FIELD_REQ(FIELD_BIT_FLATTENER);
        if (due(machine)) {
            enter_state(machine, M_FORM_FLAT_UP);
        }
        break;
    case M_FORM_FLAT_UP:
        machine->request = 0;
        if (!due(machine)) {
            break;
        }
        if (machine->calibrating) {
            machine->samples = (uint16_t)(machine->samples + 1);
            if (machine->samples >= machine->sample_target) {
                machine->calibrating = 0;
                enter_state(machine, M_MAINT);
            } else {
                enter_state(machine, M_FORM_PLUNGE);
            }
            break;
        }
        enter_state(machine, M_FORM_COUNT);
        break;
    case M_FORM_COUNT:
        machine->request = 0;
        if (!due(machine)) {
            break;
        }
        if (machine->pieces >= machine->snapshot.target_pieces) {
            machine->after_count = M_BATCH_DONE;
        } else {
            machine->after_count = M_FORM_PLUNGE;
        }
        if (machine->snapshot.forming_retract_ms > 0) {
            enter_state(machine, M_FORM_RETRACT);
        } else {
            enter_state(machine, machine->after_count);
        }
        break;
    case M_FORM_RETRACT:
        if (!machine->centering) {
            machine->request = FIELD_REQ(FIELD_BIT_PLUNGER_RETRACT);
            if (due(machine)) {
                machine->request = 0;
                machine->centering = 1;
                arm(machine, 0);
            }
        } else if (due(machine)) {
            enter_state(machine, machine->after_count);
        }
        break;
    case M_DOOR_CLOSE:
        machine->request = FIELD_REQ(FIELD_BIT_DOOR_CLOSE);
        if (machine->door_closed_cached) {
            machine->request = 0;
            enter_state(machine, machine->door_return);
            break;
        }
        if (due(machine)) {
            raise_alarm(machine, ALARM_DOOR_CLOSE_TIMEOUT);
            enter_state(machine, M_FAULT);
        }
        break;
    case M_STOPPING:
        machine->request = 0;
        if (due(machine)) {
            enter_state(machine, M_STOPPED);
        }
        break;
    case M_MAINT:
        machine->request = machine->jog_held;
        if ((machine->request & FIELD_REQ(FIELD_BIT_MIXER)) != 0 && !machine->door_closed_cached) {
            machine->request = (uint16_t)(machine->request & (uint16_t)~FIELD_REQ(FIELD_BIT_MIXER));
        }
        break;
    case M_IDLE:
    case M_STOPPED:
    case M_FAULT:
    case M_ESTOP:
    case M_BATCH_DONE:
    case M_XFER_CONFIRM:
        machine->request = 0;
        break;
    case M_BOOT:
    default:
        break;
    }
}

static void service(machine_t *machine)
{
    int guard;

    if (machine->entered) {
        run_body(machine);
    }
    for (guard = 0; guard < 6 && machine->entered == 0; guard++) {
        machine_state_t before = machine->state;
        run_entry(machine);
        if (machine->state == before) {
            machine->entered = 1;
        }
    }
}

static void apply(machine_t *machine)
{
    uint16_t request = machine->force_on ? machine->force_request : machine->request;
    uint16_t image = 0xFFFF;
    int result;

    machine->force_on = 0;
    if (machine->state == M_MAINT && (request & FIELD_REQ(FIELD_BIT_MIXER)) != 0 && !machine->door_closed_cached) {
        request = (uint16_t)(request & (uint16_t)~FIELD_REQ(FIELD_BIT_MIXER));
    }
    result = field_io_apply(&machine->io, request, &image);
    machine->image = image;
    if (result == FIELD_IO_EXCLUSION && machine->state != M_MAINT) {
        raise_alarm(machine, ALARM_OUTPUT_EXCLUSION);
        enter_state(machine, M_FAULT);
        run_entry(machine);
        machine->entered = 1;
        machine->request = 0;
        field_io_apply(&machine->io, 0, &machine->image);
    }
    machine->kicks++;
}

static void globals(machine_t *machine)
{
    if (!field_io_safety_run(&machine->io)) {
        if (machine->state != M_ESTOP) {
            raise_alarm(machine, ALARM_ESTOP_ACTIVE);
            enter_state(machine, M_ESTOP);
        }
        return;
    }
    if (machine->state != M_ESTOP && machine->state != M_FAULT) {
        if (!machine->io.link_ok) {
            raise_alarm(machine, ALARM_I2C_IO_FAILURE);
            enter_state(machine, M_FAULT);
            return;
        }
        if (!field_io_air_ok(&machine->io)) {
            raise_alarm(machine, ALARM_AIR_LOW);
            enter_state(machine, M_FAULT);
            return;
        }
        if (!field_io_drives_healthy(&machine->io)) {
            raise_alarm(machine, ALARM_DRIVE_FAULT);
            enter_state(machine, M_FAULT);
            return;
        }
    }
    if (field_io_stop_requested(&machine->io) && is_production(machine->state)) {
        enter_state(machine, M_STOPPING);
    }
}

static void take_command(machine_t *machine)
{
    machine_command_t command;
    const recipe_blob_t *recipe;
    recipe_blob_t updated;
    alarm_code_t save_alarm = ALARM_NONE;
    uint32_t hint = 0;
    int hint_result;

    if (machine->q_count == 0) {
        return;
    }
    command = machine->queue[machine->q_head];
    machine->q_head = (machine->q_head + 1) % MACHINE_CMD_QUEUE;
    machine->q_count--;

    switch (command.cmd) {
    case CMD_START:
        if (machine->state != M_IDLE) {
            return;
        }
        if (!machine->cfg.commissioned) {
            machine->banner = ALARM_UNCOMMISSIONED;
            return;
        }
        enter_state(machine, M_PRECHECK);
        break;
    case CMD_STOP:
        if (is_production(machine->state)) {
            enter_state(machine, M_STOPPING);
        }
        break;
    case CMD_RESET:
        if (machine->state == M_ESTOP) {
            if (field_io_safety_run(&machine->io)) {
                machine->alarm = ALARM_NONE;
                enter_state(machine, M_IDLE);
            }
            break;
        }
        if (machine->state == M_STOPPED || (machine->state == M_FAULT && fault_cause_clear(machine))) {
            machine->alarm = ALARM_NONE;
            enter_state(machine, M_IDLE);
        }
        break;
    case CMD_ACK:
        if (machine->state == M_BATCH_DONE) {
            machine->interrupted = 0;
            if (machine->store != 0) {
                config_store_checkpoint(machine->store, machine->pieces, machine->cycles, 0);
            }
            enter_state(machine, M_IDLE);
        }
        break;
    case CMD_CLOSE_DOOR:
        if (machine->state == M_IDLE || machine->state == M_BATCH_DONE) {
            machine->door_return = machine->state;
            enter_state(machine, M_DOOR_CLOSE);
        }
        break;
    case CMD_BARREL_READY:
        if (machine->state == M_XFER_CONFIRM) {
            enter_state(machine, M_FORM_PLUNGE);
        }
        break;
    case CMD_MAINT_ENTER:
        if (machine->state == M_IDLE && machine->cfg.maint_pin != 0 && command.arg == (uint16_t)machine->cfg.maint_pin) {
            enter_state(machine, M_MAINT);
        }
        break;
    case CMD_MAINT_EXIT:
        if (machine->state == M_MAINT && machine->jog_held == 0) {
            enter_state(machine, M_IDLE);
        }
        break;
    case CMD_JOG_PRESS:
        if (machine->state != M_MAINT) {
            return;
        }
        if (machine->jog_held != 0 && (conflicts(machine->jog_held, command.arg) || machine->jog_held != command.arg)) {
            return;
        }
        if ((command.arg & FIELD_REQ(FIELD_BIT_MIXER)) != 0 && !machine->door_closed_cached) {
            return;
        }
        machine->jog_held = command.arg;
        machine->request = command.arg;
        break;
    case CMD_JOG_RELEASE:
        if (machine->state == M_MAINT) {
            machine->jog_held = 0;
            machine->request = 0;
        }
        break;
    case CMD_CAL_START:
        if (machine->state != M_MAINT) {
            return;
        }
        if (machine->cfg.sample_count < 1 || machine->cfg.sample_count > 20) {
            return;
        }
        recipe = selected_recipe(machine);
        if (recipe != 0) {
            machine->snapshot = *recipe;
        }
        machine->calibrating = 1;
        machine->samples = 0;
        machine->sample_target = machine->cfg.sample_count;
        enter_state(machine, M_FORM_PLUNGE);
        break;
    case CMD_CAL_HINT:
        if (machine->state != M_MAINT) {
            return;
        }
        recipe = selected_recipe(machine);
        if (recipe == 0) {
            return;
        }
        hint_result = calibration_hint_ms(recipe->forming_pulse_ms, recipe->target_grams, command.arg2, command.arg, machine->cfg.pulse_min_ms, machine->cfg.pulse_max_ms, &hint);
        if (hint_result != 0) {
            raise_alarm(machine, ALARM_SETTINGS_REJECTED);
            machine->hint_ready = 0;
            return;
        }
        machine->hint_ms = hint;
        machine->hint_ready = 1;
        break;
    case CMD_CAL_CANCEL:
        machine->hint_ready = 0;
        break;
    case CMD_CAL_CONFIRM:
        if (machine->state != M_MAINT || !machine->hint_ready || machine->store == 0) {
            return;
        }
        recipe = selected_recipe(machine);
        if (recipe == 0) {
            return;
        }
        updated = *recipe;
        updated.forming_pulse_ms = machine->hint_ms;
        if (config_store_save_recipe(machine->store, machine->cfg.selected_slot, &updated, &save_alarm) != 0) {
            raise_alarm(machine, save_alarm);
            return;
        }
        machine->hint_ready = 0;
        break;
    default:
        break;
    }
}

static void finish_boot(machine_t *machine)
{
    if (!field_io_safety_run(&machine->io)) {
        raise_alarm(machine, ALARM_ESTOP_ACTIVE);
        enter_state(machine, M_ESTOP);
    } else if (!machine->io.link_ok) {
        raise_alarm(machine, ALARM_I2C_IO_FAILURE);
        enter_state(machine, M_FAULT);
    } else {
        enter_state(machine, M_IDLE);
    }
    service(machine);
    apply(machine);
}

void machine_boot(machine_t *machine, config_store_t *store, const field_io_status_t *io, uint32_t now_ms)
{
    alarm_code_t boot_alarm = ALARM_NONE;

    memset(machine, 0, sizeof(*machine));
    machine->store = store;
    machine->now_ms = now_ms;
    machine->last_ms = now_ms;
    machine->state = M_BOOT;
    if (store != 0) {
        config_store_load(store, &boot_alarm);
        machine->cfg = store->cfg;
        machine->batch = store->batch;
        machine->pieces = store->batch.pieces;
        machine->cycles = store->batch.cycles;
        machine->interrupted = store->batch.interrupted;
        if (boot_alarm == ALARM_NVS_CORRUPT) {
            raise_alarm(machine, ALARM_NVS_CORRUPT);
        }
        if (boot_alarm == ALARM_UNCOMMISSIONED || !machine->cfg.commissioned) {
            machine->banner = ALARM_UNCOMMISSIONED;
        }
    }
    machine_set_io(machine, io);
    sample_door(machine);
    finish_boot(machine);
}

void machine_set_io(machine_t *machine, const field_io_status_t *io)
{
    uint16_t output_polarity = machine->cfg.output_polarity;
    uint16_t input_polarity = machine->cfg.input_polarity;

    if (io != 0) {
        machine->io = *io;
    }
    machine->io.output_polarity = output_polarity;
    machine->io.input_polarity = input_polarity;
}

int machine_post(machine_t *machine, machine_cmd_t cmd, uint16_t arg, uint32_t arg2)
{
    int tail;

    if (machine->q_count >= MACHINE_CMD_QUEUE) {
        machine->command_rejected = 1;
        return -1;
    }
    tail = (machine->q_head + machine->q_count) % MACHINE_CMD_QUEUE;
    machine->queue[tail].cmd = cmd;
    machine->queue[tail].arg = arg;
    machine->queue[tail].arg2 = arg2;
    machine->q_count++;
    return 0;
}

void machine_force_request(machine_t *machine, uint16_t request)
{
    machine->force_request = request;
    machine->force_on = 1;
}

void machine_poll(machine_t *machine, uint32_t now_ms)
{
    machine->now_ms = now_ms;
    sample_door(machine);
    take_command(machine);
    globals(machine);
    service(machine);
    apply(machine);
    machine->last_ms = now_ms;
}

machine_state_t machine_state(const machine_t *machine)
{
    return machine->state;
}

const char *machine_state_name(machine_state_t state)
{
    switch (state) {
    case M_BOOT: return "BOOT";
    case M_IDLE: return "IDLE";
    case M_PRECHECK: return "PRECHECK";
    case M_STOPPED: return "STOPPED";
    case M_FAULT: return "FAULT";
    case M_ESTOP: return "ESTOP";
    case M_MAINT: return "MAINT";
    case M_DOOR_CLOSE: return "DOOR_CLOSE";
    case M_MIX_RUN: return "MIX_RUN";
    case M_XFER_DOOR_PARTIAL: return "XFER_DOOR_PARTIAL";
    case M_XFER_DOOR_CENTER_1: return "XFER_DOOR_CENTER_1";
    case M_XFER_MIX_1: return "XFER_MIX_1";
    case M_XFER_PUSH_1: return "XFER_PUSH_1";
    case M_XFER_PLUNGER_CENTER_1: return "XFER_PLUNGER_CENTER_1";
    case M_XFER_RETRACT_1: return "XFER_RETRACT_1";
    case M_XFER_DOOR_FULL: return "XFER_DOOR_FULL";
    case M_XFER_DOOR_CENTER_2: return "XFER_DOOR_CENTER_2";
    case M_XFER_MIX_2: return "XFER_MIX_2";
    case M_XFER_PUSH_2: return "XFER_PUSH_2";
    case M_XFER_PLUNGER_CENTER_2: return "XFER_PLUNGER_CENTER_2";
    case M_XFER_RETRACT_2: return "XFER_RETRACT_2";
    case M_XFER_CONFIRM: return "XFER_CONFIRM";
    case M_FORM_PLUNGE: return "FORM_PLUNGE";
    case M_FORM_PLUNGE_CENTER: return "FORM_PLUNGE_CENTER";
    case M_FORM_CUT_EXTEND: return "FORM_CUT_EXTEND";
    case M_FORM_CUT_DWELL: return "FORM_CUT_DWELL";
    case M_FORM_CUT_RETRACT: return "FORM_CUT_RETRACT";
    case M_FORM_CONVEY: return "FORM_CONVEY";
    case M_FORM_FLAT_DOWN: return "FORM_FLAT_DOWN";
    case M_FORM_FLAT_DWELL: return "FORM_FLAT_DWELL";
    case M_FORM_FLAT_UP: return "FORM_FLAT_UP";
    case M_FORM_COUNT: return "FORM_COUNT";
    case M_FORM_RETRACT: return "FORM_RETRACT";
    case M_BATCH_DONE: return "BATCH_DONE";
    case M_STOPPING: return "STOPPING";
    default: return "UNKNOWN";
    }
}

alarm_code_t machine_alarm(const machine_t *machine) { return machine->alarm; }
alarm_code_t machine_banner(const machine_t *machine) { return machine->banner; }
uint16_t machine_image(const machine_t *machine) { return machine->image; }
uint32_t machine_pieces(const machine_t *machine) { return machine->pieces; }
uint32_t machine_cycles(const machine_t *machine) { return machine->cycles; }
int machine_interrupted(const machine_t *machine) { return machine->interrupted; }
int machine_commissioned(const machine_t *machine) { return machine->cfg.commissioned; }
uint16_t machine_samples(const machine_t *machine) { return machine->samples; }
uint32_t machine_hint(const machine_t *machine) { return machine->hint_ms; }
int machine_command_rejected(const machine_t *machine) { return machine->command_rejected; }
uint32_t machine_watchdog_kicks(const machine_t *machine) { return machine->kicks; }
int machine_saw_count(const machine_t *machine) { return machine->saw_count; }
