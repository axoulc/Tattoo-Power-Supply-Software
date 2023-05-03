#ifndef POWER_H
#define POWER_H

#include "tattoo_types.h"

#define DELAY_POWER_LOOP_MS 200

void power_task(void *pvParameters);
void check_inputs(power_state_t *power_state, output_config_t *out1, output_config_t *out2);
void change_power_state(power_state_t next_state, output_config_t *out1, output_config_t *out2);

#endif