#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"
#include "power.h"
#include "display.h"

#include "stusb4500.h"

TaskHandle_t power_task_handle = NULL;
TaskHandle_t display_task_handle = NULL;

int main(void) {
    configure_clock();
    configure_gpio();
    configure_encoder();
    configure_spi();
    configure_usart();
    configure_i2c();
    configure_adc();

    //stusb4500_begin(&stusb);
    //uint8_t pdo = stusb4500_get_PDO_number(&stusb);
    //float voltage = stusb4500_get_voltage(&stusb, pdo);

    //printf("PDO: %d | Voltage: %f\n", pdo, voltage);

    xTaskCreate(power_task, "Power", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 2, &power_task_handle);
    xTaskCreate(display_task, "Display", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &display_task_handle);

    vTaskStartScheduler();

    // Test DACX3202
    /*
    dacx3202_init(&dacx3202);
    dacx3202_power_up(&dacx3202, DACX3202_DAC_0);
    dacx3202_power_up(&dacx3202, DACX3202_DAC_1);
    dacx3202_set_voltage(&dacx3202, DACX3202_DAC_0, 3.0);
    dacx3202_set_voltage(&dacx3202, DACX3202_DAC_1, 3.0);

    gpio_clear(EN_SMPS_Port, EN_SMPS_Pin);

    gpio_set(SW1_Port, SW1_Pin);
    gpio_set(SW2_Port, SW2_Pin);
    gpio_clear(SW1_Port, SW1_Pin);
    gpio_clear(SW2_Port, SW2_Pin);

    gpio_set(EN_SMPS_Port, EN_SMPS_Pin);
    */
    
    while (1) {
        __asm__("nop");
    }

    return 0;
}