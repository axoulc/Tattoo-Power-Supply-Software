#include "display.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"
#include "u8g2.h"
#include "mui_u8g2.h"

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
    u8g2_t u8g2_i, *u8g2 = &u8g2_i;

    mui_t mui_i, *mui = &mui_i;
  
    muif_t muif_list[] = {
        //MUIF_VARIABLE("BN", NULL, mui_u8g2_btn_exit_wm_fi)};
        MUIF_U8G2_FONT_STYLE(0, u8g2_font_helvR08_tr),   /* define style 0 */
        MUIF_U8G2_LABEL(),                               /* allow MUI_LABEL macro */
        MUIF_BUTTON("BN", mui_u8g2_btn_exit_wm_fi)
    };

    fds_t fds_data[] =
        //MUI_FORM(1)
        //MUI_XYT("BN", 64, 30, " Select Me ");
        MUI_FORM(1)                            /* This will start the definition of form 1 */
        MUI_STYLE(0)                           /* select the font defined with style 0 */
        MUI_LABEL(5, 15, "Hello U8g2")         /* place text at postion x=5, y=15 */
        MUI_XYT("BN",64, 30, " Exit ");  

    u8g2_Setup_sh1106_128x64_noname_1(u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi_stm32, u8x8_gpio_and_delay_stm32);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    //u8g2_SetFont(u8g2, u8g2_font_helvR08_tr);

    mui_Init(mui, u8g2, fds_data, muif_list, sizeof(muif_list)/sizeof(muif_t));
    mui_GotoForm(mui, 1, 0);

    for (;;) {
        u8g2_FirstPage(u8g2);
        do {
            mui_Draw(mui);
        } while (u8g2_NextPage(u8g2));
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void u8x8_byte_4wire_hw_spi_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    uint8_t *data;
    switch (msg) {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while (arg_int > 0) {
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
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(arg_int / portTICK_PERIOD_MS);
            break;
        case U8X8_MSG_GPIO_CS:
            set_cs_pin(arg_int);
            break;
        case U8X8_MSG_GPIO_DC:
            set_dc_pin(arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            set_rst_pin(arg_int);
            break;
        default:
            u8x8_SetGPIOResult(u8x8, 1);  // default return value
            break;
    }
    return 1;
}