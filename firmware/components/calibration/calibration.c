#include "calibration.h"

uint32_t piece_estimate(uint16_t target_grams)
{
    if (target_grams == 0) {
        return 0;
    }
    return 15000u / target_grams;
}

uint32_t cycle_estimate(uint16_t target_grams, uint8_t nozzle_count)
{
    uint32_t pieces;

    if (nozzle_count == 0) {
        return 0;
    }
    pieces = piece_estimate(target_grams);
    return pieces / nozzle_count;
}

int calibration_hint_ms(uint32_t pulse_ms, uint16_t target_grams, uint32_t total_grams, uint32_t sample_count, uint32_t pulse_min_ms, uint32_t pulse_max_ms, uint32_t *hint_ms)
{
    uint32_t average;
    uint32_t step;
    uint32_t hint;

    if (sample_count == 0 || total_grams == 0 || hint_ms == 0) {
        return ALARM_SETTINGS_REJECTED;
    }
    average = total_grams / sample_count;
    step = pulse_ms / 10u;
    hint = pulse_ms;
    if (average > target_grams) {
        if (step > hint) {
            hint = 0;
        } else {
            hint -= step;
        }
    } else if (average < target_grams) {
        hint += step;
    }
    if (hint < pulse_min_ms) {
        hint = pulse_min_ms;
    }
    if (hint > pulse_max_ms) {
        hint = pulse_max_ms;
    }
    *hint_ms = hint;
    return 0;
}
