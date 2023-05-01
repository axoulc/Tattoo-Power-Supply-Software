#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"
#include "semphr.h"
#include "power.h"
#include "display.h"

TaskHandle_t power_task_handle = NULL;
TaskHandle_t display_task_handle = NULL;
SemaphoreHandle_t gpiob_sem_handle = NULL;
SemaphoreHandle_t i2c_sem_handle = NULL;

/**
 * @brief Configure the MCU peripherals and proceed to launch the FreeRTOS scheduler and tasks.
 * 
 * @return int 
 */
int main(void) {
    configure_mcu();

    gpiob_sem_handle = xSemaphoreCreateBinary();
    if (gpiob_sem_handle != NULL)
        xSemaphoreGive(gpiob_sem_handle);
    i2c_sem_handle = xSemaphoreCreateBinary();
    if (i2c_sem_handle != NULL)
        xSemaphoreGive(i2c_sem_handle);

    xTaskCreate(power_task, "Power", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 2, &power_task_handle);
    xTaskCreate(display_task, "Display", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &display_task_handle);

    vTaskStartScheduler();

    return 0;
}