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
bool handle_event_display(mui_t *ui, encoder_t *encoder);

TaskHandle_t DisplayTaskHandle = NULL;

void display_init_task(void) {
    BaseType_t xReturned;
    xReturned = xTaskCreate(display_task, "Display", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &DisplayTaskHandle);
}

void display_task(void *pvParameters) {
    uint32_t count = 0, last_count = 0;
    char buffer[16];
    u8g2_t u8g2_i, *u8g2 = &u8g2_i;
    encoder_t encoder = {0, 0};
    bool is_redraw = true;

/*
    mui_t mui_i, *mui = &mui_i;

    uint8_t number = 0;
  
    muif_t muif_list[] = {
        //MUIF_VARIABLE("BN", NULL, mui_u8g2_btn_exit_wm_fi)};
        MUIF_U8G2_FONT_STYLE(0, u8g2_font_helvR08_tr),
        MUIF_U8G2_LABEL(),
        MUIF_U8G2_U8_MIN_MAX_STEP("B0", &number, 0, 20, 2, MUI_MMS_2X_BAR|MUI_MMS_SHOW_VALUE, mui_u8g2_u8_bar_wm_mud_pf)
    };

    fds_t fds_data[] =
        MUI_FORM(1)
        MUI_STYLE(0)
        MUI_LABEL(1,40, "Bar: ")
        MUI_XY("B0",40, 40)
        MUI_GOTO(64, 59, 90, " Ok ")
        ;
*/
    u8g2_Setup_sh1106_128x64_noname_1(u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi_stm32, u8x8_gpio_and_delay_stm32);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_helvR08_tr);
    u8g2_SetFontMode(u8g2, 0);
    //mui_Init(mui, u8g2, fds_data, muif_list, sizeof(muif_list)/sizeof(muif_t));
    //mui_GotoForm(mui, 1, 0);

    for (;;) {
        if (is_redraw) {
            u8g2_FirstPage(u8g2);
            do {
                u8g2_DrawFrame(u8g2, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
                u8g2_DrawHLine(u8g2, 0, TOP_BOX_HEIGHT, DISPLAY_WIDTH);
                u8g2_DrawVLine(u8g2, 64, TOP_BOX_HEIGHT, DISPLAY_HEIGHT - TOP_BOX_HEIGHT);
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawStr(u8g2, 2, TOP_BOX_HEIGHT - 2, "Tattoo Machine");
                u8g2_SetDrawColor(u8g2, 0);
                u8g2_DrawStr(u8g2, 106, TOP_BOX_HEIGHT - 2, "20V");
                u8g2_SetDrawColor(u8g2, 1);
                //mui_Draw(mui);
            
            } while (u8g2_NextPage(u8g2));
            is_redraw = false;
        }
        //is_redraw = handle_event_display(mui, &encoder);
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

bool handle_event_display(mui_t *ui, encoder_t *encoder) {
    bool ret = false;
    if (!get_encoder_sw()) {
        mui_SendSelect(ui);
        ret = true;
    }
    if (!get_encoder_clk()) {
        encoder->current = get_encoder_count() >> 2;
        if (encoder->current == encoder->last) {
            return ret;
        } else if ((encoder->current - encoder->last) > 0) {
            mui_NextField(ui);
            encoder->last = encoder->current;
            ret = true;
        } else if ((encoder->current - encoder->last) < 0) {
            mui_PrevField(ui);
            encoder->last = encoder->current;
            ret = true;
        }
    }
    return ret;
}