#include "display.h"
#include "peripherals.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "u8x8.h"

void display_task(void *pvParameters);
void u8x8_byte_4wire_hw_spi_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

TaskHandle_t DisplayTaskHandle = NULL;

void display_init_task(void) {
    BaseType_t xReturned;
    xReturned = xTaskCreate(display_task, "Display", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &DisplayTaskHandle);
}

void display_task(void *pvParameters) {
    uint32_t count = 0, last_count = 0;
    char buffer[16];
    u8x8_t u8x8_i, *u8x8 = &u8x8_i;
    u8x8_Setup(u8x8, u8x8_d_sh1106_128x64_noname, u8x8_cad_001, u8x8_byte_4wire_hw_spi_stm32, u8x8_gpio_and_delay_stm32);
    u8x8_InitDisplay(u8x8);
	u8x8_SetPowerSave(u8x8,0);
	u8x8_SetFont(u8x8, u8x8_font_7x14B_1x2_f);

	u8x8_ClearDisplay(u8x8);

    for(;;) {
        count = get_encoder_count();
        if (count != last_count) {
            last_count = count;
            u8x8_ClearDisplay(u8x8);
            sprintf(buffer, "%d", count);
            u8x8_DrawString(u8x8, 0, 0, buffer);
        }
        vTaskDelay(250/portTICK_PERIOD_MS);
    }
}

void u8x8_byte_4wire_hw_spi_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	uint8_t *data;
    switch (msg) {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while( arg_int > 0 ) {
                spi_send_byte((uint8_t)*data);
                data++;
                arg_int--;
            } 
            break;
        case U8X8_MSG_BYTE_INIT:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            break;
        case U8X8_MSG_BYTE_SET_DC:
            u8x8_gpio_SetDC(u8x8, arg_int);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            break;
        default:
            return 0;
    }
    return 1;
}

void u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch (msg) {
        case U8X8_MSG_DELAY_MILLI:  // delay arg_int * 1 milli second
            vTaskDelay(arg_int / portTICK_PERIOD_MS);
            break;
        case U8X8_MSG_GPIO_CS:  // CS (chip select) pin: Output level in arg_int
            set_cs_pin(arg_int);
            break;
        case U8X8_MSG_GPIO_DC:  // DC (data/cmd, A0, register select) pin: Output level in arg_int
            set_dc_pin(arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:  // Reset pin: Output level in arg_int
            set_rst_pin(arg_int);
            break;  // arg_int=1: Input dir with pullup high for I2C data pin
        default:
            u8x8_SetGPIOResult(u8x8, 1);  // default return value
            break;
    }
    return 1;
}