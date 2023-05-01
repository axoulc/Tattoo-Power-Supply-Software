#include "power.h"

#include "FreeRTOS.h"
#include "peripherals.h"
#include "task.h"
#include "DACx3202.h"

dacx3202_t dacx3202 = {
    .addr = DACX3202_7B_ADDR(0x00),
    .i2c_read = read_i2c,
    .i2c_write = write_i2c,
    .vref = 3.3
};

void power_task(void *pvParameters) {
    (void)pvParameters;

    power_state_t power_state = POWER_OFF;

    output_config_t out1 = {
        .output = OUT_1,
        .voltage = 0,
        .type = DC,
        .footswitch = false,
        .handswitch = false
    };

    output_config_t out2 = {
        .output = OUT_2,
        .voltage = 0,
        .type = DC,
        .footswitch = false,
        .handswitch = false
    };

    set_rtos_pin(EN_SMPS_Port, EN_SMPS_Pin, 1);
    set_rtos_pin(SW1_Port, SW1_Pin, 0);
    set_rtos_pin(SW2_Port, SW2_Pin, 0);

    if (0) { // TODO: Check pdo and alert display task if is 5V
        vTaskSuspend( NULL );
    }

    if (dacx3202_init(&dacx3202) != 0) {
        // TODO: Handle error
    }

    dacx3202_power_up(&dacx3202, DACX3202_DAC_0);
    dacx3202_power_up(&dacx3202, DACX3202_DAC_1);
    dacx3202_set_voltage(&dacx3202, DACX3202_DAC_0, 3.0);
    dacx3202_set_voltage(&dacx3202, DACX3202_DAC_1, 3.0);
    set_rtos_pin(EN_SMPS_Port, EN_SMPS_Pin, 0);

    for (;;) {
        check_inputs(&power_state, &out1, &out2);
        vTaskDelay(pdMS_TO_TICKS(DELAY_POWER_LOOP_MS));
    }
    
}

void check_inputs(power_state_t *power_state, output_config_t *out1, output_config_t *out2) {
    uint16_t footswitch_state = get_footswitch_state();
    uint16_t handswitch_state = get_handswitch_state();

    switch (*power_state) {
        case POWER_OFF:
            if (footswitch_state) {
                change_power_state(POWER_ON_FOOT, out1, out2);
                *power_state = POWER_ON_FOOT;
            } else if (handswitch_state) {
                change_power_state(POWER_ON_HAND, out1, out2);
                *power_state = POWER_ON_HAND;
            }
            break;
        case POWER_ON_FOOT:
            if (footswitch_state == 0) {
                change_power_state(POWER_OFF, out1, out2);
                *power_state = POWER_OFF;
            }
            break;
        case POWER_ON_HAND:
            if (handswitch_state) {
                change_power_state(POWER_OFF, out1, out2);
                *power_state = POWER_OFF;
            }
            break;
        default:
            break;
    }
}

void change_power_state(power_state_t next_state, output_config_t *out1, output_config_t *out2) {
    switch (next_state) {
        case POWER_OFF:
            set_rtos_pin(SW1_Port, SW1_Pin, 0);
            set_rtos_pin(SW2_Port, SW2_Pin, 0);
            break;
        case POWER_ON_FOOT:
            if (out1->footswitch) {
                // TODO : Set voltage
                if (out1->type == DC) {
                    set_rtos_pin(SW2_Port, SW2_Pin, 1);
                } else {

                }
            }
            if (out2->footswitch) {
                // TODO : Set voltage
                if (out2->type == DC) {
                    set_rtos_pin(SW1_Port, SW1_Pin, 1);
                } else {

                }
            }
            break;
        case POWER_ON_HAND:
            if (out1->handswitch) {
                // TODO : Set voltage
                if (out1->type == DC) {
                    set_rtos_pin(SW2_Port, SW2_Pin, 1);
                } else {
                        
                }
            }
            if (out2->handswitch) {
                // TODO : Set voltage
                if (out2->type == DC) {
                    set_rtos_pin(SW1_Port, SW1_Pin, 1);
                } else {

                }
            }
            break;
        default:
            break;
    }
}