#include <stdint.h>
#include <stdio.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "peripherals.h"

void vTaskCode(void *pvParameters);

TaskHandle_t xHandle = NULL;

int main(void) {
	configure_clock();
	configure_gpio();
	configure_encoder();
	configure_spi();
	configure_usart();
	configure_i2c();
	configure_adc();

	xTaskCreate(vTaskCode, "NAME", 256, (void *)1, tskIDLE_PRIORITY + 1, &xHandle);

	vTaskStartScheduler();

	while (1) {
		__asm__("nop");
	}

	return 0;
}

void vTaskCode(void *pvParameters)
{
	int adc_value = 0;
	for (;;)
	{
		adc_value = read_adc_native(0);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}