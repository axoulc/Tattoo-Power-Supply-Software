#include "display.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "peripherals.h"
#include "task.h"
#include "u8g2.h"

void display_task(void *pvParameters);
uint8_t u8x8_byte_4wire_hw_spi_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void handle_event_display(encoder_t *encoder);

void draw_display(display_state_t current_state, u8g2_t *u8g2, output_data_t *out1, output_data_t *out2);
void draw_main(u8g2_t *u8g2, output_data_t *out1, output_data_t *out2);
void draw_set_config(u8g2_t *u8g2, output_data_t *out1, output_data_t *out2);

bool handle_action_display(display_state_t current_state, encoder_t *encoder, output_data_t *out1, output_data_t *out2);
bool handle_action_main(encoder_t *encoder, output_data_t *out1, output_data_t *out2);
bool handle_action_set_config(encoder_t *encoder, output_data_t *out1, output_data_t *out2);

void draw_button(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t is_selected, const char *label);
void draw_pane(u8g2_t *u8g2, output_data_t *data);

TaskHandle_t DisplayTaskHandle = NULL;

/**
 * @brief Initialize display task
 * 
 */
void display_init_task(void) {
    //TODO Creer mutex & sémaphore pour hardware
    xTaskCreate(display_task, "Display", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &DisplayTaskHandle);
}

/**
 * @brief Display task
 * 
 * @param pvParameters 
 */
void display_task(void *pvParameters) {
    (void)pvParameters; //TODO : Modifier pour passer en param le PDO

    output_data_t out1 = {OUT_1, 0, true, 0};
    output_data_t out2 = {OUT_2, 0, false, DISPLAY_WIDTH / 2};
    u8g2_t u8g2_i, *u8g2 = &u8g2_i;
    encoder_t encoder = {0, 0, EVENT_NONE};
    display_state_t state = DISPLAY_MAIN;
    bool is_redraw = true;

    u8g2_Setup_sh1106_128x64_noname_1(u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi_stm32, u8x8_gpio_and_delay_stm32);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_helvR08_tr);

    for (;;) {
        if (is_redraw) {
            draw_display(state, u8g2, &out1, &out2);
            is_redraw = false;
        }
        handle_event_display(&encoder);
        is_redraw = handle_action_display(state, &encoder, &out1, &out2);
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Hardware SPI interface for u8g2
 * 
 * @param u8x8 
 * @param msg 
 * @param arg_int 
 * @param arg_ptr 
 * @return uint8_t 
 */
uint8_t u8x8_byte_4wire_hw_spi_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
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

/**
 * @brief Hardware Delay and GPIO interface for u8g2
 * 
 * @param u8x8 
 * @param msg 
 * @param arg_int 
 * @param arg_ptr 
 * @return uint8_t 
 */
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
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

/**
 * @brief Handle event from encoder
 * 
 * @param encoder 
 */
void handle_event_display(encoder_t *encoder) {
    if (!get_encoder_sw()) {
        vTaskDelay(250 / portTICK_PERIOD_MS);
        encoder->event = EVENT_SELECT;
        return;
    }
    if (!get_encoder_rot()) {
        encoder->current = get_encoder_count() >> 2;
        if (encoder->current == encoder->last) {
            encoder->event = EVENT_NONE;
            return;
        }
        if ((int16_t)(encoder->current - encoder->last) > 0) {
            encoder->last = encoder->current;
            encoder->event = EVENT_NEXT;
            return;
        }
        if ((int16_t)(encoder->current - encoder->last) < 0) {
            encoder->last = encoder->current;
            encoder->event = EVENT_PREV;
            return;
        }
    }
}

/**
 * @brief Draw display for current state
 * 
 * @param current_state 
 * @param u8g2 
 * @param out1 
 * @param out2 
 */
void draw_display(display_state_t current_state, u8g2_t *u8g2, output_data_t *out1, output_data_t *out2) {
    switch (current_state) {
        case DISPLAY_INIT:
        case DISPLAY_MAIN:
            draw_main(u8g2, out1, out2);
            break;
        case DISPLAY_SET_CONFIG:
            draw_set_config(u8g2, out1, out2);
            break;
        default:
            break;
    }
}

/**
 * @brief Draw main display
 * 
 * @param u8g2 
 * @param out1 
 * @param out2 
 */
void draw_main(u8g2_t *u8g2, output_data_t *out1, output_data_t *out2) {
    u8g2_FirstPage(u8g2);
    do {
        u8g2_DrawFrame(u8g2, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        u8g2_DrawHLine(u8g2, 0, TOP_BOX_HEIGHT, DISPLAY_WIDTH);
        u8g2_DrawVLine(u8g2, DISPLAY_WIDTH / 2, TOP_BOX_HEIGHT, DISPLAY_HEIGHT - TOP_BOX_HEIGHT);
        u8g2_DrawStr(u8g2, 2, TOP_BOX_HEIGHT - 2, "Tattoo Machine");
        u8g2_DrawStr(u8g2, 106, TOP_BOX_HEIGHT - 2, "20V");

        draw_pane(u8g2, out1);
        draw_pane(u8g2, out2);
    } while (u8g2_NextPage(u8g2));
}

/**
 * @brief Draw the set config screen
 * 
 * @param u8g2 
 * @param out1 
 * @param out2 
 */
void draw_set_config(u8g2_t *u8g2, output_data_t *out1, output_data_t *out2) {

}

/**
 * @brief Handle the display state for actions
 * 
 * @param current_state 
 * @param encoder 
 * @param out1 
 * @param out2 
 * @return true 
 * @return false 
 */
bool handle_action_display(display_state_t current_state, encoder_t *encoder, output_data_t *out1, output_data_t *out2) {
    bool ret = false;
    switch (current_state) {
        case DISPLAY_MAIN:
            ret = handle_action_main(encoder, out1, out2);
            break;
        case DISPLAY_SET_CONFIG:
            ret = handle_action_set_config(encoder, out1, out2);
            break;
        default:
            break;
    }
    return ret;
}

/**
 * @brief Handle the main display state
 * 
 * @param encoder 
 * @param out1 
 * @param out2 
 * @return true 
 * @return false 
 */
bool handle_action_main(encoder_t *encoder, output_data_t *out1, output_data_t *out2) {
    bool ret = false;
    if (encoder->event == EVENT_SELECT) {
        if (out1->is_set_selected) {
            out1->is_set_selected = false;
            out2->is_set_selected = true;
        } else {
            out1->is_set_selected = true;
            out2->is_set_selected = false;
        }
        encoder->event = EVENT_NONE;
        ret = true;
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
        ret = true;
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
        ret = true;
    }
    return ret;
}

/**
 * @brief Handle action for set config
 * 
 * @param encoder 
 * @param out1 
 * @param out2 
 * @return true 
 * @return false 
 */
bool handle_action_set_config(encoder_t *encoder, output_data_t *out1, output_data_t *out2) {
    return false;
}

/**
 * @brief Draw a button
 * 
 * @param u8g2 
 * @param x 
 * @param y 
 * @param is_selected 
 * @param label 
 */
void draw_button(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t is_selected, const char *label) {
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

/**
 * @brief Draw a pane with voltage and set button
 * 
 * @param u8g2
 * @param data  
 */
void draw_pane(u8g2_t *u8g2, output_data_t *data) {
    static char buf[10];
    sprintf(buf, "%d.%dV", data->voltage / 10, data->voltage % 10);
    u8g2_DrawStr(u8g2, 22 + data->x_offset, 40, buf);
    draw_button(u8g2, 20 + data->x_offset, 45, data->is_set_selected, " Set ");
}