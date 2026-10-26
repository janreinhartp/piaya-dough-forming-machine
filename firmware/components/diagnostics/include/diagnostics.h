#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "machine.h"

void diagnostics_note(machine_state_t state, alarm_code_t alarm, uint32_t pieces);

#endif
