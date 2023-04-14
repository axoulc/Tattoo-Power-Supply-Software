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
void handle_event_display(encoder_t *encoder);
void handle_action_display(encoder_t *encoder, output_data_t *out1, output_data_t *out2);
void draw_button(u8g2_t * u8g2, uint8_t x, uint8_t y, uint8_t is_selected, const char *label);
void draw_pane(u8g2_t * u8g2, output_data_t * data);

TaskHandle_t DisplayTaskHandle = NULL;
bool is_redraw = true;

void display_init_task(void) {
    BaseType_t xReturned;
    xReturned = xTaskCreate(display_task, "Display", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &DisplayTaskHandle);
}

void display_task(void *pvParameters) {
    output_data_t out1 = {OUT_1, 0, true, 0};
    output_data_t out2 = {OUT_2, 0, false, DISPLAY_WIDTH / 2};
    u8g2_t u8g2_i, *u8g2 = &u8g2_i;
    encoder_t encoder = {0, 0, EVENT_NONE};

    u8g2_Setup_sh1106_128x64_noname_1(u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi_stm32, u8x8_gpio_and_delay_stm32);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_helvR08_tr);

    for (;;) {
        if (is_redraw) {
            u8g2_FirstPage(u8g2);
            do {
                u8g2_DrawFrame(u8g2, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
                u8g2_DrawHLine(u8g2, 0, TOP_BOX_HEIGHT, DISPLAY_WIDTH);
                u8g2_DrawVLine(u8g2, DISPLAY_WIDTH / 2, TOP_BOX_HEIGHT, DISPLAY_HEIGHT - TOP_BOX_HEIGHT);
                u8g2_DrawStr(u8g2, 2, TOP_BOX_HEIGHT - 2, "Tattoo Machine");
                u8g2_DrawStr(u8g2, 106, TOP_BOX_HEIGHT - 2, "20V");

                draw_pane(u8g2, &out1);
                draw_pane(u8g2, &out2);
            } while (u8g2_NextPage(u8g2));
            is_redraw = false;
        }
        handle_event_display(&encoder);
        handle_action_display(&encoder, &out1, &out2);
        vTaskDelay(25 / portTICK_PERIOD_MS);
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

void handle_event_display(encoder_t *encoder) {
    if (!get_encoder_sw()) {
        vTaskDelay(250 / portTICK_PERIOD_MS);
        encoder->event = EVENT_SELECT;
        return;
    }
    if (!get_encoder_clk()) {
        encoder->current = get_encoder_count() >> 2;
        printf("encoder->current = %d", encoder->current);
        if (encoder->current == encoder->last) {
            encoder->event = EVENT_NONE;
            return;
        }
        if ((encoder->current - encoder->last) > 0) {
            encoder->last = encoder->current;
            encoder->event = EVENT_NEXT;
            return;
        }
        if ((encoder->current - encoder->last) < 0) {
            encoder->last = encoder->current;
            encoder->event = EVENT_PREV;
            return;
        }
    }
}

void handle_action_display(encoder_t *encoder, output_data_t *out1, output_data_t *out2) {
    if (encoder->event == EVENT_SELECT) {
        if (out1->is_set_selected) {
            out1->is_set_selected = false;
            out2->is_set_selected = true;
        } else {
            out1->is_set_selected = true;
            out2->is_set_selected = false;
        }
        encoder->event = EVENT_NONE;
        is_redraw = true;
    } else if (encoder->event == EVENT_NEXT) {
        if (out1->is_set_selected) {
            if (out1->voltage < 100) {
                out1->voltage++;
            }
        } else {
            if (out2->voltage < 100) {
                out2->voltage++;
            }
        }
        encoder->event = EVENT_NONE;
        is_redraw = true;
    } else if (encoder->event == EVENT_PREV) {
        if (out1->is_set_selected) {
            if (out1->voltage > 0) {
                out1->voltage--;
            }
        } else {
            if (out2->voltage > 0) {
                out2->voltage--;
            }
        }
        encoder->event = EVENT_NONE;
        is_redraw = true;
    }
}

void draw_button(u8g2_t * u8g2, uint8_t x, uint8_t y, uint8_t is_selected, const char *label) {
    uint8_t w = u8g2_GetStrWidth(u8g2, label) + 4;
    uint8_t h = u8g2_GetMaxCharHeight(u8g2) + 2;
    u8g2_DrawFrame(u8g2, x, y, w, h);
    if (is_selected) {
        u8g2_DrawBox(u8g2, x + 1, y + 1, w - 2, h - 2);
        u8g2_SetDrawColor(u8g2, 0);
    }
    u8g2_DrawStr(u8g2, x + 2, y + h - 2, label);
    u8g2_SetDrawColor(u8g2, 1);
}

void draw_pane(u8g2_t * u8g2, output_data_t * data) {
    static char buf[10];
    sprintf(buf, "%d.%dV", data->voltage / 10, data->voltage % 10);
    u8g2_DrawStr(u8g2, 22 + data->x_offset, 40, buf);
    draw_button(u8g2, 20 + data->x_offset, 45, data->is_set_selected, " Set ");
}