#ifndef RUNTIME_H
#define RUNTIME_H

#include "machine.h"

void runtime_start(void);
void runtime_lock(void);
void runtime_unlock(void);
const machine_t *runtime_machine(void);
int runtime_post(machine_cmd_t cmd, uint16_t arg, uint32_t arg2);

#endif
