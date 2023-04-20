#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"

#include <libopencm3/stm32/i2c.h>
#include "display.h"
#include "DACx3202.h"
#include "stusb4500.h"

void vTaskCode(void *pvParameters);


uint8_t read_i2c(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t len);
uint8_t write_i2c(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t len);

uint8_t write_i2c_stusb(uint8_t addr, uint16_t reg, uint8_t *data_w, uint16_t len);
uint8_t read_i2c_stusb(uint8_t addr, uint16_t reg, uint8_t *data_r, uint16_t len);

dacx3202_t dacx3202 = {
    .addr = DACX3202_7B_ADDR(0x00),
    .i2c_read = read_i2c,
    .i2c_write = write_i2c,
    .vref = 3.3
};

stusb4500_t stusb = {
    .addr = 0x28,
    .i2c_read = read_i2c_stusb,
    .i2c_write = write_i2c_stusb,
    .hard_reset = false
};

int main(void) {
    configure_clock();
    configure_gpio();
    configure_encoder();
    configure_spi();
    configure_usart();
    configure_i2c();
    configure_adc();

    stusb4500_begin(&stusb);
    uint8_t pdo = stusb4500_get_PDO_number(&stusb);
    //float voltage = stusb4500_get_voltage(&stusb, pdo);

    //printf("PDO: %d | Voltage: %f\n", pdo, voltage);

    
    display_init_task();

    //xTaskCreate(vTaskCode, "NAME", 256, (void *)1, tskIDLE_PRIORITY + 1, &xHandle);

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

uint8_t write_i2c_stusb(uint8_t addr, uint16_t reg, uint8_t *data_w, uint16_t len) {
    uint8_t tx[3] = { 0 };
    tx[0] = reg;
    for (int i = 0; i < len; i++) {
        tx[i + 1] = data_w[i];
    }
    i2c_transfer7(I2C1, addr, tx, len + 1, NULL, 0);
    return 0;
}

uint8_t read_i2c_stusb(uint8_t addr, uint16_t reg, uint8_t *data_r, uint16_t len) {
    uint8_t tx = reg;
    i2c_transfer7(I2C1, addr, &tx, 1, data_r, len);
    return 0;
}