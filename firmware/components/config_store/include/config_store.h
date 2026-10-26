#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <stdint.h>

#include "alarm.h"

#define RECIPE_SCHEMA 1
#define RECIPE_NAME_LEN 24
#define RECIPE_SLOTS 8
#define ALARM_RING_LEN 32

typedef struct recipe_blob {
    uint16_t schema;
    char name[RECIPE_NAME_LEN];
    uint32_t mix_ms;
    uint32_t xfer_mix_1_ms;
    uint32_t xfer_mix_2_ms;
    uint32_t door_partial_ms;
    uint32_t door_full_ms;
    uint32_t door_close_ms;
    uint32_t push_1_ms;
    uint32_t retract_1_ms;
    uint32_t push_2_ms;
    uint32_t retract_2_ms;
    uint32_t forming_pulse_ms;
    uint32_t forming_retract_ms;
    uint32_t cutter_extend_ms;
    uint32_t cutter_dwell_ms;
    uint32_t cutter_retract_ms;
    uint32_t conveyor_ms;
    uint32_t flat_down_ms;
    uint32_t flat_dwell_ms;
    uint32_t flat_up_ms;
    uint16_t target_grams;
    uint8_t nozzle_count;
    uint16_t target_pieces;
} recipe_blob_t;

typedef struct machine_cfg {
    uint16_t schema;
    uint32_t door_close_margin_ms;
    uint32_t debounce_ms;
    uint32_t pulse_min_ms;
    uint32_t pulse_max_ms;
    uint16_t sample_count;
    uint16_t output_polarity;
    uint16_t input_polarity;
    int commissioned;
    uint32_t maint_pin;
    uint32_t eng_pin;
    int selected_slot;
} machine_cfg_t;

typedef struct batch_meta {
    uint16_t schema;
    int interrupted;
    uint32_t pieces;
    uint32_t cycles;
} batch_meta_t;

typedef struct alarm_record {
    uint16_t code;
    uint16_t state;
    char message[80];
} alarm_record_t;

typedef struct config_store {
    int corrupt;
    int missing;
    int fail_save;
    int fail_checkpoint;
    recipe_blob_t recipes[RECIPE_SLOTS];
    machine_cfg_t cfg;
    batch_meta_t batch;
    alarm_record_t alarms[ALARM_RING_LEN];
    int alarm_next;
    int alarm_count;
} config_store_t;

void config_store_factory(config_store_t *store);
int config_store_recipe_valid(const recipe_blob_t *recipe, const machine_cfg_t *cfg);
int config_store_load(config_store_t *store, alarm_code_t *boot_alarm);
int config_store_save_recipe(config_store_t *store, int slot, const recipe_blob_t *recipe, alarm_code_t *alarm);
int config_store_checkpoint(config_store_t *store, uint32_t pieces, uint32_t cycles, int interrupted);
void config_store_push_alarm(config_store_t *store, alarm_code_t code, uint16_t state);
const recipe_blob_t *config_store_recipe(const config_store_t *store, int slot);

#endif
