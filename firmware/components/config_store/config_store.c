#include "config_store.h"

#include <string.h>

static int between(uint32_t value, uint32_t low, uint32_t high)
{
    return value >= low && value <= high;
}

static void copy_text(char *dest, size_t dest_len, const char *src)
{
    size_t i;

    if (dest_len == 0) {
        return;
    }
    for (i = 0; i + 1 < dest_len && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static void fill_name(char *name, const char *text)
{
    memset(name, 0, RECIPE_NAME_LEN);
    copy_text(name, RECIPE_NAME_LEN, text);
}

void config_store_factory(config_store_t *store)
{
    int slot;
    recipe_blob_t recipe;

    memset(store, 0, sizeof(*store));
    memset(&recipe, 0, sizeof(recipe));
    recipe.schema = RECIPE_SCHEMA;
    fill_name(recipe.name, "Factory");
    recipe.mix_ms = 1000;
    recipe.xfer_mix_1_ms = 100;
    recipe.xfer_mix_2_ms = 100;
    recipe.door_partial_ms = 100;
    recipe.door_full_ms = 100;
    recipe.door_close_ms = 100;
    recipe.push_1_ms = 100;
    recipe.retract_1_ms = 0;
    recipe.push_2_ms = 100;
    recipe.retract_2_ms = 0;
    recipe.forming_pulse_ms = 100;
    recipe.forming_retract_ms = 0;
    recipe.cutter_extend_ms = 100;
    recipe.cutter_dwell_ms = 0;
    recipe.cutter_retract_ms = 100;
    recipe.conveyor_ms = 100;
    recipe.flat_down_ms = 100;
    recipe.flat_dwell_ms = 0;
    recipe.flat_up_ms = 100;
    recipe.target_grams = 10;
    recipe.nozzle_count = 1;
    recipe.target_pieces = 1;

    for (slot = 0; slot < RECIPE_SLOTS; slot++) {
        store->recipes[slot] = recipe;
    }

    store->cfg.schema = RECIPE_SCHEMA;
    store->cfg.door_close_margin_ms = 500;
    store->cfg.debounce_ms = 30;
    store->cfg.pulse_min_ms = 100;
    store->cfg.pulse_max_ms = 5000;
    store->cfg.sample_count = 4;
    store->cfg.commissioned = 0;
    store->batch.schema = RECIPE_SCHEMA;
}

int config_store_recipe_valid(const recipe_blob_t *recipe, const machine_cfg_t *cfg)
{
    size_t name_len;

    if (recipe == 0 || cfg == 0) {
        return 0;
    }
    if (recipe->schema != RECIPE_SCHEMA || cfg->schema != RECIPE_SCHEMA) {
        return 0;
    }
    name_len = strlen(recipe->name);
    if (name_len < 1 || name_len >= RECIPE_NAME_LEN) {
        return 0;
    }
    if (!between(recipe->mix_ms, 1000, 1800000)) {
        return 0;
    }
    if (!between(recipe->xfer_mix_1_ms, 100, 600000) || !between(recipe->xfer_mix_2_ms, 100, 600000)) {
        return 0;
    }
    if (!between(recipe->door_partial_ms, 100, 60000) || !between(recipe->door_full_ms, 100, 60000) || !between(recipe->door_close_ms, 100, 60000)) {
        return 0;
    }
    if (!between(recipe->push_1_ms, 100, 60000) || !between(recipe->push_2_ms, 100, 60000)) {
        return 0;
    }
    if (!between(recipe->retract_1_ms, 0, 60000) || !between(recipe->retract_2_ms, 0, 60000)) {
        return 0;
    }
    if (!between(cfg->pulse_min_ms, 0, 120000) || !between(cfg->pulse_max_ms, 0, 120000) || cfg->pulse_min_ms > cfg->pulse_max_ms) {
        return 0;
    }
    if (!between(recipe->forming_pulse_ms, cfg->pulse_min_ms, cfg->pulse_max_ms)) {
        return 0;
    }
    if (!between(recipe->forming_retract_ms, 0, 60000)) {
        return 0;
    }
    if (!between(recipe->cutter_extend_ms, 100, 30000) || !between(recipe->cutter_retract_ms, 100, 30000)) {
        return 0;
    }
    if (!between(recipe->cutter_dwell_ms, 0, 30000)) {
        return 0;
    }
    if (!between(recipe->conveyor_ms, 100, 60000)) {
        return 0;
    }
    if (!between(recipe->flat_down_ms, 100, 30000) || !between(recipe->flat_up_ms, 100, 30000)) {
        return 0;
    }
    if (!between(recipe->flat_dwell_ms, 0, 30000)) {
        return 0;
    }
    if (!between(recipe->target_grams, 10, 200)) {
        return 0;
    }
    if (recipe->nozzle_count < 1 || recipe->nozzle_count > 2) {
        return 0;
    }
    if (!between(recipe->target_pieces, 1, 2000)) {
        return 0;
    }
    if (recipe->mix_ms > 3600000 || recipe->door_close_ms > 120000 || recipe->flat_up_ms > 120000) {
        return 0;
    }
    return 1;
}

static int store_is_bad(const config_store_t *store)
{
    if (store->corrupt || store->missing) {
        return 1;
    }
    if (store->cfg.schema != RECIPE_SCHEMA || store->batch.schema != RECIPE_SCHEMA) {
        return 1;
    }
    if (store->recipes[0].schema != RECIPE_SCHEMA) {
        return 1;
    }
    return 0;
}

int config_store_load(config_store_t *store, alarm_code_t *boot_alarm)
{
    int missing;

    if (boot_alarm != 0) {
        *boot_alarm = ALARM_NONE;
    }
    missing = store->missing;
    if (!store_is_bad(store)) {
        return 0;
    }
    config_store_factory(store);
    if (missing) {
        if (boot_alarm != 0) {
            *boot_alarm = ALARM_UNCOMMISSIONED;
        }
        return 0;
    }
    if (boot_alarm != 0) {
        *boot_alarm = ALARM_NVS_CORRUPT;
    }
    return -1;
}

int config_store_save_recipe(config_store_t *store, int slot, const recipe_blob_t *recipe, alarm_code_t *alarm)
{
    if (alarm != 0) {
        *alarm = ALARM_NONE;
    }
    if (slot < 0 || slot >= RECIPE_SLOTS || !config_store_recipe_valid(recipe, &store->cfg)) {
        if (alarm != 0) {
            *alarm = ALARM_SETTINGS_REJECTED;
        }
        return -1;
    }
    if (store->fail_save) {
        if (alarm != 0) {
            *alarm = ALARM_NVS_SAVE_FAIL;
        }
        return -1;
    }
    store->recipes[slot] = *recipe;
    return 0;
}

int config_store_checkpoint(config_store_t *store, uint32_t pieces, uint32_t cycles, int interrupted)
{
    if (store->fail_checkpoint) {
        return -1;
    }
    store->batch.schema = RECIPE_SCHEMA;
    store->batch.pieces = pieces;
    store->batch.cycles = cycles;
    store->batch.interrupted = interrupted;
    return 0;
}

void config_store_push_alarm(config_store_t *store, alarm_code_t code, uint16_t state)
{
    alarm_record_t *record;

    record = &store->alarms[store->alarm_next];
    memset(record, 0, sizeof(*record));
    record->code = (uint16_t)code;
    record->state = state;
    copy_text(record->message, sizeof(record->message), alarm_message(code));
    store->alarm_next = (store->alarm_next + 1) % ALARM_RING_LEN;
    if (store->alarm_count < ALARM_RING_LEN) {
        store->alarm_count++;
    }
}

const recipe_blob_t *config_store_recipe(const config_store_t *store, int slot)
{
    if (slot < 0 || slot >= RECIPE_SLOTS) {
        return 0;
    }
    return &store->recipes[slot];
}
