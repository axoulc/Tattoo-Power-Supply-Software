#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"

#include <libopencm3/stm32/i2c.h>
 #include "DACx3202.h"

void vTaskCode(void *pvParameters);


uint8_t read_i2c(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t len);
uint8_t write_i2c(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t len);

dacx3202_t dacx3202 = {
    .addr = DACX3202_7B_ADDR(0x00),
    .i2c_read = read_i2c,
    .i2c_write = write_i2c,
    .vref = 3.3
};

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
    dacx3202_init(&dacx3202);
    dacx3202_power_up(&dacx3202, DACX3202_DAC_0);
    dacx3202_power_up(&dacx3202, DACX3202_DAC_1);
    dacx3202_set_voltage(&dacx3202, DACX3202_DAC_0, 2.0);
    dacx3202_set_voltage(&dacx3202, DACX3202_DAC_1, 2.0);

    gpio_clear(EN_SMPS_Port, EN_SMPS_Pin);

    dacx3202_set_voltage(&dacx3202, DACX3202_DAC_0, 0.5);
    dacx3202_set_voltage(&dacx3202, DACX3202_DAC_1, 0.5);
    
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

uint8_t read_i2c(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t len) {
    uint8_t tx = reg;
    i2c_transfer7(I2C1, addr, &tx, 1, buffer, len);
    return 0;
}

uint8_t write_i2c(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t len) {
    uint8_t tx[3] = { 0 };
    tx[0] = reg;
    for (int i = 0; i < len; i++) {
        tx[i + 1] = buffer[i];
    }
    i2c_transfer7(I2C1, addr, tx, len + 1, NULL, 0);
    return 0;
}