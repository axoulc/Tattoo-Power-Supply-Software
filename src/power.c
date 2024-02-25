#include "power.h"

#include "DACx3202.h"
#include "FreeRTOS.h"
#include "peripherals.h"
#include "queue.h"
#include "semphr.h"
#include "stusb4500.h"
#include "task.h"

uint16_t convert_voltage_into_raw_dac(uint16_t voltage, output_t output);

extern QueueHandle_t power_state_queue;
extern QueueHandle_t output_config_queue;

stusb4500_t stusb4500 = {
    .addr = 0x28,
    .i2c_read = read_i2c,
    .i2c_write = write_i2c,
    .hard_reset = false};

dacx3202_t dacx3202 = {
    .addr = DACX3202_7B_ADDR(0x00),
    .i2c_read = read_i2c,
    .i2c_write = write_i2c,
    .vref = 3.3};

/**
 * @brief Power task. This task is responsible for controlling the power supplies.
 *
 * @param pvParameters
 */
void power_task(void *pvParameters) {
    (void)pvParameters;

    power_state_t power_state = POWER_OFF;

    output_config_t out1 = {
        .output = OUT_1,
        .voltage = 20,
        .enable = true};

    output_config_t out2 = {
        .output = OUT_2,
        .voltage = 20,
        .enable = false};

    output_config_t buffer_out;

    set_rtos_pin(EN_SMPS_Port, EN_SMPS_Pin, 1);
    set_rtos_pin(SW1_Port, SW1_Pin, 0);
    set_rtos_pin(SW2_Port, SW2_Pin, 0);

    //vTaskDelay(pdMS_TO_TICKS(2000)); 

    // stusb4500_begin(&stusb4500);
    // uint8_t pdo = stusb4500_get_PDO_number(&stusb4500);
    // float voltage = stusb4500_get_voltage(&stusb4500, pdo);

    if (0) {  // TODO: Check pdo and alert display task if is 5V
        vTaskSuspend(NULL);
    }

    if (dacx3202_init(&dacx3202) != 0) {
        // TODO: Handle error
    }
    dacx3202_power_up(&dacx3202, DACX3202_DAC_0);
    dacx3202_power_up(&dacx3202, DACX3202_DAC_1);
    dacx3202_set_value(&dacx3202, DACX3202_DAC_1, convert_voltage_into_raw_dac(out1.voltage, OUT_1));
    dacx3202_set_value(&dacx3202, DACX3202_DAC_0, convert_voltage_into_raw_dac(out1.voltage, OUT_2));
    set_rtos_pin(EN_SMPS_Port, EN_SMPS_Pin, 0);

    for (;;) {
        if (xQueueReceive(output_config_queue, &buffer_out, (TickType_t)10) == pdTRUE) {
            if (buffer_out.output == OUT_1) {
                out1 = buffer_out;
            } else if (buffer_out.output == OUT_2) {
                out2 = buffer_out;
            }
        }
        check_inputs(&power_state, &out1, &out2);
        vTaskDelay(pdMS_TO_TICKS(DELAY_POWER_LOOP_MS));
    }
}

/**
 * @brief Check the footswitch and handswitch states and change the power state if necessary.
 *
 * @param power_state
 * @param out1
 * @param out2
 */
void check_inputs(power_state_t *power_state, output_config_t *out1, output_config_t *out2) {
    uint16_t footswitch_state = !get_footswitch_state();
    uint16_t handswitch_state = !get_handswitch_state();

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
            } else if (footswitch_state) {
                change_power_state(POWER_OFF, out1, out2);
                while(!get_footswitch_state());
                *power_state = POWER_OFF;
            }
            break;
        default:
            break;
    }
}

/**
 * @brief Change the power state.
 *
 * @param next_state
 * @param out1
 * @param out2
 */
void change_power_state(power_state_t next_state, output_config_t *out1, output_config_t *out2) {
    switch (next_state) {
        case POWER_OFF:
            set_rtos_pin(SW1_Port, SW1_Pin, 0);
            set_rtos_pin(SW2_Port, SW2_Pin, 0);
            set_rtos_pin(LED_Port, LED_Pin, 0);
            break;
        case POWER_ON_FOOT:
            if (out1->enable) {
                dacx3202_set_value(&dacx3202, DACX3202_DAC_1, convert_voltage_into_raw_dac(out1->voltage, OUT_1));
                set_rtos_pin(SW2_Port, SW2_Pin, 1);
            }
            if (out2->enable) {
                dacx3202_set_value(&dacx3202, DACX3202_DAC_0, convert_voltage_into_raw_dac(out2->voltage, OUT_2));
                set_rtos_pin(SW1_Port, SW1_Pin, 1);
            }
            set_rtos_pin(LED_Port, LED_Pin, 1);
            break;
        case POWER_ON_HAND:
            if (out1->enable) {
                dacx3202_set_value(&dacx3202, DACX3202_DAC_1, convert_voltage_into_raw_dac(out1->voltage, OUT_1));
                set_rtos_pin(SW2_Port, SW2_Pin, 1);
            }
            if (out2->enable) {
                dacx3202_set_value(&dacx3202, DACX3202_DAC_0, convert_voltage_into_raw_dac(out2->voltage, OUT_2));
                set_rtos_pin(SW1_Port, SW1_Pin, 1);
            }
            set_rtos_pin(LED_Port, LED_Pin, 1);
            break;
        default:
            break;
    }
    xQueueSend(power_state_queue, &next_state, 0);
}

/**
 * @brief Convert a voltage in mV into a raw value for the DAC.
 *
 * @param voltage
 * @return uint16_t
 */
uint16_t convert_voltage_into_raw_dac(uint16_t voltage, output_t output) {
    float voltage_buf = ((float)voltage) / 10.0;
    if (output == OUT_1) {
        voltage_buf = voltage_buf / OUT1_CALIB_FACTOR - OUT1_CALIB_OFFSET;
    } else if (output == OUT_2) {
        voltage_buf = voltage_buf / OUT2_CALIB_FACTOR - OUT2_CALIB_OFFSET;
    }
    float value_dec = (float)(1 << (dacx3202.type == DAC53202_10b ? 10 : 12)) * (R1 * R2 * V_REF + R1 * R3 * V_REF + R2 * R3 * (-voltage_buf + V_REF)) / (R1 * R2 * dacx3202.vref);
    return (uint16_t)value_dec;
}