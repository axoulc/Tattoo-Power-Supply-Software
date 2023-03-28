#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"

#include <libopencm3/stm32/i2c.h>
 #include "DACx3202.h"

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

    //xTaskCreate(vTaskCode, "NAME", 256, (void *)1, tskIDLE_PRIORITY + 1, &xHandle);

    //vTaskStartScheduler();

    // Test DACX3202
    uint8_t tx = DACX3202_REG_GENERAL_STATUS;
    uint8_t rx = 0;
    i2c_transfer7(I2C1, DACX3202_7B_ADDR, &tx, 1, &rx, 1);

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