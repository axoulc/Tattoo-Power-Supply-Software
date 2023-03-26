#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"

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

void vTaskCode(void *pvParameters) {
    int value = 0, last = 0;
    for (;;) {
        value = get_encoder_count();
        if (value != last) {
            printf("Encod : %d\r\n", value);
            last = value;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}