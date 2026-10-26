#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>

#include "alarm.h"

uint32_t piece_estimate(uint16_t target_grams);
uint32_t cycle_estimate(uint16_t target_grams, uint8_t nozzle_count);
int calibration_hint_ms(uint32_t pulse_ms, uint16_t target_grams, uint32_t total_grams, uint32_t sample_count, uint32_t pulse_min_ms, uint32_t pulse_max_ms, uint32_t *hint_ms);

#endif
