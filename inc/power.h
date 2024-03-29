#ifndef POWER_H
#define POWER_H

#include "tattoo_types.h"

#define DELAY_POWER_LOOP_MS 100

#define R1 8450.0
#define R2 1000.0
#define R3 1650.0
#define V_REF 1.23
#define V_REFDAC 3.3
#define OUT1_CALIB_FACTOR 0.994468
#define OUT2_CALIB_FACTOR 0.995322
#define OUT1_CALIB_OFFSET 0.376613
#define OUT2_CALIB_OFFSET 0.432452

void power_task(void *pvParameters);
void check_inputs(power_state_t *power_state, output_config_t *out1, output_config_t *out2);
void change_power_state(power_state_t next_state, output_config_t *out1, output_config_t *out2);

#endif