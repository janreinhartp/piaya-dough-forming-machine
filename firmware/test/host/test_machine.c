#include <stdio.h>
#include <string.h>

#include "calibration.h"
#include "machine.h"

static int g_failures;
static config_store_t g_store;
static machine_t g_machine;
static field_io_status_t g_io;
static uint32_t g_now;

static void expect_true(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL %s (state %s alarm %d pieces %lu)\n", name, machine_state_name(machine_state(&g_machine)), (int)machine_alarm(&g_machine), (unsigned long)machine_pieces(&g_machine));
        g_failures++;
    }
}

static field_io_status_t healthy(void)
{
    field_io_status_t status;

    memset(&status, 0, sizeof(status));
    status.link_ok = 1;
    return status;
}

static void boot_with(int commissioned)
{
    memset(&g_store, 0, sizeof(g_store));
    config_store_factory(&g_store);
    g_store.cfg.commissioned = commissioned;
    g_store.cfg.maint_pin = 1111;
    g_store.cfg.debounce_ms = 0;
    g_store.cfg.door_close_margin_ms = 0;
    g_store.cfg.sample_count = 2;
    g_store.recipes[0].target_grams = 50;
    g_store.recipes[0].forming_pulse_ms = 1000;
    g_store.recipes[0].nozzle_count = 2;
    g_store.recipes[0].target_pieces = 2;
    g_io = healthy();
    g_now = 1000;
    machine_boot(&g_machine, &g_store, &g_io, g_now);
}

static void poll(void)
{
    machine_poll(&g_machine, g_now);
}

static void advance(uint32_t dt)
{
    g_now += dt;
    poll();
}

static uint32_t state_dt(machine_state_t state)
{
    const recipe_blob_t *recipe = config_store_recipe(&g_store, 0);

    switch (state) {
    case M_MIX_RUN: return recipe->mix_ms;
    case M_XFER_DOOR_PARTIAL: return recipe->door_partial_ms;
    case M_XFER_MIX_1: return recipe->xfer_mix_1_ms;
    case M_XFER_PUSH_1: return recipe->push_1_ms;
    case M_XFER_RETRACT_1: return recipe->retract_1_ms;
    case M_XFER_DOOR_FULL: return recipe->door_full_ms;
    case M_XFER_MIX_2: return recipe->xfer_mix_2_ms;
    case M_XFER_PUSH_2: return recipe->push_2_ms;
    case M_XFER_RETRACT_2: return recipe->retract_2_ms;
    case M_FORM_PLUNGE: return recipe->forming_pulse_ms;
    case M_FORM_CUT_EXTEND: return recipe->cutter_extend_ms;
    case M_FORM_CUT_DWELL: return recipe->cutter_dwell_ms;
    case M_FORM_CUT_RETRACT: return recipe->cutter_retract_ms;
    case M_FORM_CONVEY: return recipe->conveyor_ms;
    case M_FORM_FLAT_DOWN: return recipe->flat_down_ms;
    case M_FORM_FLAT_DWELL: return recipe->flat_dwell_ms;
    case M_FORM_FLAT_UP: return recipe->flat_up_ms;
    case M_FORM_RETRACT: return recipe->forming_retract_ms;
    case M_DOOR_CLOSE: return recipe->door_close_ms + g_store.cfg.door_close_margin_ms;
    default: return 0;
    }
}

static void leave(void)
{
    advance(state_dt(machine_state(&g_machine)));
}

static void expect_state(machine_state_t state, const char *name)
{
    if (machine_state(&g_machine) != state) {
        printf("FAIL %s expected %s got %s\n", name, machine_state_name(state), machine_state_name(machine_state(&g_machine)));
        g_failures++;
    }
}

static void run_to_confirm(void)
{
    machine_post(&g_machine, CMD_START, 0, 0);
    poll();
    expect_state(M_PRECHECK, "start enters precheck");
    leave();
    expect_state(M_MIX_RUN, "precheck enters mix");
    expect_true(machine_interrupted(&g_machine), "mix sets the interrupted flag");
    leave();
    expect_state(M_XFER_DOOR_PARTIAL, "mix goes to partial open");
    leave();
    expect_state(M_XFER_DOOR_CENTER_1, "partial open centers the door");
    leave();
    expect_state(M_XFER_MIX_1, "center goes to transfer mix 1");
    leave();
    expect_state(M_XFER_PUSH_1, "transfer mix 1 pushes");
    leave();
    expect_state(M_XFER_PLUNGER_CENTER_1, "push 1 centers the plunger");
    leave();
    expect_state(M_XFER_DOOR_FULL, "zero retract 1 skips to full open");
    leave();
    expect_state(M_XFER_DOOR_CENTER_2, "full open centers the door");
    leave();
    expect_state(M_XFER_MIX_2, "second center goes to transfer mix 2");
    leave();
    expect_state(M_XFER_PUSH_2, "transfer mix 2 pushes");
    leave();
    expect_state(M_XFER_PLUNGER_CENTER_2, "push 2 centers the plunger");
    leave();
    expect_state(M_XFER_CONFIRM, "zero retract 2 waits for barrel ready");
}

static void run_to_flat_up(void)
{
    machine_post(&g_machine, CMD_BARREL_READY, 0, 0);
    poll();
    expect_state(M_FORM_PLUNGE, "barrel ready plunges");
    expect_true(machine_pieces(&g_machine) == 0, "plunge does not count");
    leave();
    expect_state(M_FORM_PLUNGE_CENTER, "pulse centers the plunger");
    leave();
    expect_state(M_FORM_CUT_EXTEND, "center extends the cutter");
    leave();
    expect_state(M_FORM_CUT_RETRACT, "zero dwell skips to cutter retract");
    expect_true((machine_image(&g_machine) & FIELD_REQ(FIELD_BIT_CUTTER_EXTEND)) != 0, "retract leaves the extend coil off");
    leave();
    expect_state(M_FORM_CONVEY, "cutter retract runs the conveyor");
    leave();
    expect_state(M_FORM_FLAT_DOWN, "conveyor lowers the flattener");
    leave();
    expect_state(M_FORM_FLAT_DWELL, "down enters dwell even when dwell is zero");
    leave();
    expect_state(M_FORM_FLAT_UP, "dwell raises the flattener");
    expect_true(machine_pieces(&g_machine) == 0, "pieces stay zero through flattener up");
    expect_true(!machine_saw_count(&g_machine), "count state has not run yet");
}

static void test_transfer_skips_zero_retract(void)
{
    boot_with(1);
    run_to_confirm();
}

static void test_count_only_after_flat_up(void)
{
    boot_with(1);
    run_to_confirm();
    run_to_flat_up();
    leave();
    expect_state(M_FORM_COUNT, "flattener up enters count");
    expect_true(machine_pieces(&g_machine) == 2, "two nozzles add two pieces");
    expect_true(machine_saw_count(&g_machine), "count state ran");
    leave();
    expect_state(M_BATCH_DONE, "target met skips a zero forming retract");
}

static void test_stop_during_flat_up_does_not_count(void)
{
    boot_with(1);
    run_to_confirm();
    run_to_flat_up();
    machine_post(&g_machine, CMD_STOP, 0, 0);
    poll();
    expect_state(M_STOPPING, "stop during flattener up enters stopping");
    expect_true(machine_pieces(&g_machine) == 0, "stop during flattener up does not count");
    poll();
    expect_state(M_STOPPED, "inactive write enters stopped");
    expect_true(machine_image(&g_machine) == 0xFFFF, "stopped image is inactive");
}

static void test_stop_pin_not_low(void)
{
    boot_with(1);
    run_to_confirm();
    g_io.raw_pins = (uint16_t)(1u << 13);
    machine_set_io(&g_machine, &g_io);
    poll();
    expect_state(M_STOPPING, "STOP pin that is not low stops confirm");
}

static void test_door_open_during_mix(void)
{
    boot_with(1);
    machine_post(&g_machine, CMD_START, 0, 0);
    poll();
    leave();
    expect_state(M_MIX_RUN, "mix is running");
    g_io.raw_pins = (uint16_t)(1u << 9);
    machine_set_io(&g_machine, &g_io);
    poll();
    expect_state(M_FAULT, "open door during mix faults");
    expect_true(machine_alarm(&g_machine) == ALARM_DOOR_OPEN_DURING_MIX, "door alarm is DOOR_OPEN_DURING_MIX");
    expect_true((machine_image(&g_machine) & FIELD_REQ(FIELD_BIT_MIXER)) != 0, "mixer run bit is off");
}

static void test_refused_start_is_not_mix_fault(void)
{
    boot_with(1);
    g_io.raw_pins = (uint16_t)(1u << 9);
    machine_set_io(&g_machine, &g_io);
    machine_post(&g_machine, CMD_START, 0, 0);
    poll();
    leave();
    expect_state(M_IDLE, "open door at precheck returns idle");
    expect_true(machine_alarm(&g_machine) != ALARM_DOOR_OPEN_DURING_MIX, "refused start is not a mix fault");
}

static void test_safety_from_forming_is_estop(void)
{
    boot_with(1);
    run_to_confirm();
    machine_post(&g_machine, CMD_BARREL_READY, 0, 0);
    poll();
    expect_state(M_FORM_PLUNGE, "forming has started");
    g_io.raw_pins = (uint16_t)(1u << 10);
    machine_set_io(&g_machine, &g_io);
    poll();
    expect_state(M_ESTOP, "safety loss from forming enters estop");
    expect_true(machine_alarm(&g_machine) == ALARM_ESTOP_ACTIVE, "estop alarm is recorded");
    expect_true(machine_image(&g_machine) == 0xFFFF, "estop image is inactive");
    machine_post(&g_machine, CMD_RESET, 0, 0);
    poll();
    expect_state(M_ESTOP, "reset is ignored while the monitor is unhealthy");
    g_io = healthy();
    machine_set_io(&g_machine, &g_io);
    machine_post(&g_machine, CMD_RESET, 0, 0);
    poll();
    expect_state(M_IDLE, "healthy monitor and reset enter idle");
}

static void test_output_exclusion(void)
{
    boot_with(1);
    machine_post(&g_machine, CMD_START, 0, 0);
    poll();
    leave();
    machine_force_request(&g_machine, (uint16_t)(FIELD_REQ(FIELD_BIT_DOOR_OPEN) | FIELD_REQ(FIELD_BIT_DOOR_CLOSE)));
    poll();
    expect_state(M_FAULT, "automatic pair request faults");
    expect_true(machine_alarm(&g_machine) == ALARM_OUTPUT_EXCLUSION, "exclusion alarm is OUTPUT_EXCLUSION");
    expect_true(machine_image(&g_machine) == 0xFFFF, "exclusion writes the inactive image");
}

static void test_reboot_does_not_resume(void)
{
    boot_with(1);
    g_store.batch.interrupted = 1;
    g_store.batch.pieces = 4;
    g_io = healthy();
    machine_boot(&g_machine, &g_store, &g_io, g_now);
    expect_state(M_IDLE, "reboot enters idle");
    expect_true(machine_state(&g_machine) != M_FORM_PLUNGE, "reboot does not enter forming");
    expect_true(machine_interrupted(&g_machine), "interrupted count stays visible");
    expect_true(machine_pieces(&g_machine) == 4, "interrupted piece count stays visible");
}

static void test_corrupt_store(void)
{
    boot_with(1);
    g_store.corrupt = 1;
    g_store.cfg.commissioned = 1;
    machine_boot(&g_machine, &g_store, &g_io, g_now);
    expect_state(M_IDLE, "corrupt boot stays idle");
    expect_true(machine_alarm(&g_machine) == ALARM_NVS_CORRUPT, "corrupt store raises NVS_CORRUPT");
    expect_true(!machine_commissioned(&g_machine), "corrupt store clears commissioned");
    expect_true(strcmp(config_store_recipe(&g_store, 0)->name, "Factory") == 0, "corrupt store loads factory data");
}

static void test_calibration_does_not_count_production(void)
{
    int steps;

    boot_with(1);
    g_store.batch.interrupted = 1;
    g_store.batch.pieces = 4;
    machine_boot(&g_machine, &g_store, &g_io, g_now);
    machine_post(&g_machine, CMD_MAINT_ENTER, 1111, 0);
    poll();
    expect_state(M_MAINT, "maintenance pin enters maintenance");
    machine_post(&g_machine, CMD_CAL_START, 0, 0);
    poll();
    expect_state(M_FORM_PLUNGE, "calibration starts at plunge");
    for (steps = 0; steps < 30 && machine_state(&g_machine) != M_MAINT; steps++) {
        expect_true(machine_state(&g_machine) != M_FORM_COUNT, "calibration never enters count");
        leave();
    }
    expect_state(M_MAINT, "sample count returns to maintenance");
    expect_true(!machine_saw_count(&g_machine), "calibration does not run the production count");
    expect_true(machine_pieces(&g_machine) == 4, "production piece count is unchanged");
    expect_true(machine_interrupted(&g_machine), "interrupted flag is unchanged");
    expect_true(machine_samples(&g_machine) == 2, "both samples were completed");
    expect_true(machine_image(&g_machine) == 0xFFFF, "calibration ends with outputs inactive");
}

static void test_hint_waits_for_confirm(void)
{
    uint32_t hint = 1;
    const recipe_blob_t *recipe;

    boot_with(1);
    machine_post(&g_machine, CMD_MAINT_ENTER, 1111, 0);
    poll();
    machine_post(&g_machine, CMD_CAL_HINT, 2, 200);
    poll();
    expect_true(machine_hint(&g_machine) == 900, "high average shortens the pulse by 10 percent");
    recipe = config_store_recipe(&g_store, 0);
    expect_true(recipe->forming_pulse_ms == 1000, "hint does not write the recipe");
    machine_post(&g_machine, CMD_CAL_CANCEL, 0, 0);
    poll();
    expect_true(config_store_recipe(&g_store, 0)->forming_pulse_ms == 1000, "cancel leaves the pulse");
    machine_post(&g_machine, CMD_CAL_HINT, 2, 200);
    poll();
    machine_post(&g_machine, CMD_CAL_CONFIRM, 0, 0);
    poll();
    expect_true(config_store_recipe(&g_store, 0)->forming_pulse_ms == 900, "confirm stores the pulse");

    expect_true(calibration_hint_ms(1000, 50, 40, 2, 100, 5000, &hint) == 0, "low average hint succeeds");
    expect_true(hint == 1100, "low average lengthens the pulse by 10 percent");
    hint = 1234;
    expect_true(calibration_hint_ms(1000, 50, 0, 2, 100, 5000, &hint) == ALARM_SETTINGS_REJECTED, "zero weight is rejected");
    expect_true(hint == 1234, "rejected hint does not write the output");
    expect_true(calibration_hint_ms(100, 50, 200, 2, 100, 5000, &hint) == 0, "clamp hint succeeds");
    expect_true(hint == 100, "hint is clamped to the engineering minimum");
    expect_true(piece_estimate(50) == 300, "piece estimate is floor of 15000 over grams");
    expect_true(cycle_estimate(50, 2) == 150, "cycle estimate divides the piece estimate by nozzles");
}

static void test_maint_mixer_requires_door(void)
{
    boot_with(1);
    machine_post(&g_machine, CMD_MAINT_ENTER, 1111, 0);
    poll();
    g_io.raw_pins = (uint16_t)(1u << 9);
    machine_set_io(&g_machine, &g_io);
    machine_post(&g_machine, CMD_JOG_PRESS, FIELD_REQ(FIELD_BIT_MIXER), 0);
    poll();
    expect_state(M_MAINT, "rejected jog stays in maintenance");
    expect_true(machine_alarm(&g_machine) == ALARM_NONE, "rejected mixer jog raises no alarm");
    expect_true((machine_image(&g_machine) & FIELD_REQ(FIELD_BIT_MIXER)) != 0, "mixer run stays off");

    g_io = healthy();
    machine_set_io(&g_machine, &g_io);
    machine_post(&g_machine, CMD_JOG_PRESS, FIELD_REQ(FIELD_BIT_DOOR_OPEN), 0);
    poll();
    machine_post(&g_machine, CMD_JOG_PRESS, FIELD_REQ(FIELD_BIT_DOOR_CLOSE), 0);
    poll();
    expect_true(machine_alarm(&g_machine) == ALARM_NONE, "second coil raises no alarm");
    expect_true((machine_image(&g_machine) & FIELD_REQ(FIELD_BIT_DOOR_OPEN)) == 0, "held door-open coil stays");
    expect_true((machine_image(&g_machine) & FIELD_REQ(FIELD_BIT_DOOR_CLOSE)) != 0, "rejected door-close coil stays off");
}

static void test_uncommissioned_and_queue(void)
{
    int i;
    alarm_code_t alarm;

    boot_with(0);
    expect_true(machine_banner(&g_machine) == ALARM_UNCOMMISSIONED, "factory boot shows uncommissioned");
    machine_post(&g_machine, CMD_START, 0, 0);
    poll();
    expect_state(M_IDLE, "uncommissioned start stays idle");
    expect_true(machine_alarm(&g_machine) == ALARM_NONE, "uncommissioned start is not a fault");

    boot_with(1);
    for (i = 0; i < MACHINE_CMD_QUEUE; i++) {
        expect_true(machine_post(&g_machine, CMD_ACK, 0, 0) == 0, "queue accepts eight commands");
    }
    alarm = machine_alarm(&g_machine);
    expect_true(machine_post(&g_machine, CMD_START, 0, 0) != 0, "full queue rejects the new command");
    expect_true(machine_command_rejected(&g_machine), "rejection is reported to the screen");
    expect_true(machine_alarm(&g_machine) == alarm, "full queue raises no alarm");
}

static void test_checkpoint_failure_keeps_ram_count(void)
{
    boot_with(1);
    g_store.fail_checkpoint = 1;
    run_to_confirm();
    run_to_flat_up();
    leave();
    expect_state(M_FAULT, "checkpoint failure faults");
    expect_true(machine_alarm(&g_machine) == ALARM_NVS_SAVE_FAIL, "checkpoint failure raises NVS_SAVE_FAIL");
    expect_true(machine_pieces(&g_machine) == 2, "ram count is kept");
    expect_true(g_store.batch.pieces == 0, "failed checkpoint does not store the count");
}

static void test_door_close_timeout_keeps_batch(void)
{
    boot_with(1);
    run_to_confirm();
    run_to_flat_up();
    leave();
    leave();
    expect_state(M_BATCH_DONE, "batch finished");
    g_io.raw_pins = (uint16_t)(1u << 9);
    machine_set_io(&g_machine, &g_io);
    machine_post(&g_machine, CMD_CLOSE_DOOR, 0, 0);
    poll();
    expect_state(M_DOOR_CLOSE, "batch done can close the door");
    leave();
    expect_state(M_FAULT, "door close timeout faults");
    expect_true(machine_alarm(&g_machine) == ALARM_DOOR_CLOSE_TIMEOUT, "timeout alarm is DOOR_CLOSE_TIMEOUT");
    expect_true(machine_pieces(&g_machine) == 2, "finished batch record stays");
    machine_post(&g_machine, CMD_RESET, 0, 0);
    poll();
    expect_state(M_FAULT, "reset waits until the door switch reads closed");
    g_io = healthy();
    machine_set_io(&g_machine, &g_io);
    machine_post(&g_machine, CMD_RESET, 0, 0);
    poll();
    expect_state(M_IDLE, "reset after the switch closes enters idle");
    expect_true(machine_state(&g_machine) != M_FORM_PLUNGE, "the finished batch is not resumed");
}

static void test_illegal_command_is_dropped(void)
{
    boot_with(1);
    machine_post(&g_machine, CMD_START, 0, 0);
    poll();
    leave();
    expect_state(M_MIX_RUN, "mix before illegal start");
    machine_post(&g_machine, CMD_START, 0, 0);
    poll();
    expect_state(M_MIX_RUN, "illegal start is dropped");
    expect_true(machine_alarm(&g_machine) == ALARM_NONE, "illegal command raises no alarm");
}

static void test_rejected_edit_keeps_recipe(void)
{
    recipe_blob_t bad;
    alarm_code_t alarm = ALARM_NONE;

    boot_with(1);
    bad = *config_store_recipe(&g_store, 0);
    bad.target_grams = 0;
    expect_true(config_store_save_recipe(&g_store, 0, &bad, &alarm) != 0, "zero grams is rejected");
    expect_true(alarm == ALARM_SETTINGS_REJECTED, "out of range edit raises SETTINGS_REJECTED");
    expect_true(config_store_recipe(&g_store, 0)->target_grams == 50, "previous recipe stays");
}

static void test_alarm_text(void)
{
    expect_true(alarm_message(ALARM_ESTOP_ACTIVE)[0] != 0, "estop message is present");
    expect_true(alarm_recovery(ALARM_OUTPUT_EXCLUSION)[0] != 0, "exclusion recovery text is present");
}

int main(void)
{
    test_alarm_text();
    test_rejected_edit_keeps_recipe();
    test_transfer_skips_zero_retract();
    test_count_only_after_flat_up();
    test_stop_during_flat_up_does_not_count();
    test_stop_pin_not_low();
    test_door_open_during_mix();
    test_refused_start_is_not_mix_fault();
    test_safety_from_forming_is_estop();
    test_output_exclusion();
    test_reboot_does_not_resume();
    test_corrupt_store();
    test_calibration_does_not_count_production();
    test_hint_waits_for_confirm();
    test_maint_mixer_requires_door();
    test_uncommissioned_and_queue();
    test_checkpoint_failure_keeps_ram_count();
    test_door_close_timeout_keeps_batch();
    test_illegal_command_is_dropped();

    if (g_failures != 0) {
        printf("%d failed\n", g_failures);
        return 1;
    }
    printf("ok\n");
    return 0;
}
