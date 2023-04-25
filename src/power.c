#include "power.h"

#include "FreeRTOS.h"
#include "peripherals.h"
#include "task.h"

void power_task(void *pvParameters);

TaskHandle_t power_task_handle = NULL;

void power_init_task(void) {
    xTaskCreate(power_task, "Power", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 2, &power_task_handle);
}

void power_task(void *pvParameters) {
    (void)pvParameters;
    
}    