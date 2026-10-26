#include "diagnostics.h"

#include "esp_log.h"

static const char *TAG = "diagnostics";

void diagnostics_note(machine_state_t state, alarm_code_t alarm, uint32_t pieces)
{
    static machine_state_t last_state;
    static alarm_code_t last_alarm;
    static uint32_t last_pieces;
    static int primed;

    if (primed && state == last_state && alarm == last_alarm && pieces == last_pieces) {
        return;
    }
    primed = 1;
    last_state = state;
    last_alarm = alarm;
    last_pieces = pieces;
    if (alarm == ALARM_NONE) {
        ESP_LOGI(TAG, "state %s pieces %lu", machine_state_name(state), (unsigned long)pieces);
    } else {
        ESP_LOGE(TAG, "state %s alarm %s pieces %lu", machine_state_name(state), alarm_message(alarm), (unsigned long)pieces);
    }
}
