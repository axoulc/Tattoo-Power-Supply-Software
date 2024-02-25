#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "power.h"
#include "display.h"
#include "tattoo_types.h"

void error_handler(void);

TaskHandle_t power_task_handle = NULL;
TaskHandle_t display_task_handle = NULL;
SemaphoreHandle_t gpiob_sem_handle = NULL;
SemaphoreHandle_t i2c_sem_handle = NULL;
QueueHandle_t power_state_queue = NULL;
QueueHandle_t output_config_queue = NULL;

/**
 * @brief Configure the MCU peripherals and proceed to launch the FreeRTOS scheduler and tasks.
 * 
 * @return int 
 */
int main(void) {
    int i = 0;
    configure_mcu();

    for (i = 0; i < 800000; i++)	/* Wait a bit. */
			__asm__("nop");

    gpiob_sem_handle = xSemaphoreCreateBinary();
    if (gpiob_sem_handle == NULL)
        error_handler();    
    xSemaphoreGive(gpiob_sem_handle);
    i2c_sem_handle = xSemaphoreCreateBinary();
    if (i2c_sem_handle == NULL)
        error_handler();
    xSemaphoreGive(i2c_sem_handle);
    power_state_queue = xQueueCreate(1, sizeof(power_state_t));
    if (power_state_queue == NULL)
        error_handler();
    output_config_queue = xQueueCreate(1, sizeof(output_config_t));
    if (output_config_queue == NULL)
        error_handler();

    xTaskCreate(power_task, "Power", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 2, &power_task_handle);
    xTaskCreate(display_task, "Display", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &display_task_handle);

    vTaskStartScheduler();

    return 0;
}

void error_handler(void) {
    taskDISABLE_INTERRUPTS();
    vTaskSuspendAll();

    for (;;) {
        gpio_toggle(LED_Port, LED_Pin);
   		for (uint32_t i = 0; i < 800000; i++)
			__asm__("nop");
    }
}