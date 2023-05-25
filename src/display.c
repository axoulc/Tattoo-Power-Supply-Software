#include "display.h"

#include "FreeRTOS.h"
#include "peripherals.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "u8g2.h"
#include "img_tattoo.h"

uint8_t u8x8_byte_4wire_hw_spi_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void handle_event_display(encoder_t *encoder);

void draw_display(config_t *current_config, u8g2_t *u8g2, output_data_t *out1, output_data_t *out2);
void draw_main(config_t *current_config, u8g2_t *u8g2, output_data_t *out1, output_data_t *out2);
void draw_set_config(config_t *current_config, u8g2_t *u8g2);
void draw_tattoo(config_t *current_config, u8g2_t *u8g2);

void handle_action_display(config_t *current_config, encoder_t *encoder, output_data_t *out1, output_data_t *out2);
void handle_action_main(config_t *current_config, encoder_t *encoder, output_data_t *out1, output_data_t *out2);
void handle_action_set_config(config_t *current_config, encoder_t *encoder, output_data_t *out1, output_data_t *out2);
void handle_action_tattoo(config_t *current_config);

void draw_text(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t is_selected, const char *text);
void draw_button(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t is_selected, const char *label);
void draw_checkbox(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t w, uint8_t is_checked);
void draw_pane(u8g2_t *u8g2, output_data_t *data);

uint16_t read_voltage(output_t output);

extern QueueHandle_t power_state_queue;
extern QueueHandle_t output_config_queue;

/**
 * @brief Display task
 *
 * @param pvParameters
 */
void display_task(void *pvParameters) {
    (void)pvParameters;  // TODO : Modifier pour passer en param le PDO

    output_data_t out1 = {
        .output = OUT_1,
        .voltage = 20,
        .type = DC,
        .footswitch = true,
        .handswitch = false,
        .is_set_selected = true,
        .x_offset = 0};

    output_data_t out2 = {
        .output = OUT_2,
        .voltage = 20,
        .type = DC,
        .footswitch = false,
        .handswitch = false,
        .is_set_selected = false,
        .x_offset = DISPLAY_WIDTH / 2};

    encoder_t encoder = {
        .current = 0,
        .last = 0,
        .event = EVENT_NONE};

    config_t config = {
        .current_pdo = 15,
        .current_state = DISPLAY_MAIN,
        .is_redraw = true,
        .selected = false,
        .cursor_idx = CONFIG_VOLTAGE,
        .logo_blink = false,
        .logo_blink_counter = 0};

    power_state_t power_state = POWER_OFF;

    u8g2_t u8g2_i, *u8g2 = &u8g2_i;

    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for startup 3s ?

    u8g2_Setup_sh1106_128x64_noname_1(u8g2, U8G2_R0, u8x8_byte_4wire_hw_spi_stm32, u8x8_gpio_and_delay_stm32);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
    u8g2_SetFont(u8g2, u8g2_font_helvR08_tr);

    for (;;) {
        if (xQueueReceive(power_state_queue, &power_state, 0) == pdTRUE) {
            if (power_state == POWER_OFF) {
                config.current_state = DISPLAY_MAIN;
                config.is_redraw = true;
            } else {
                config.current_state = DISPLAY_TATTOO;
                config.is_redraw = true;
            }
        }
        if (config.is_redraw) {
            draw_display(&config, u8g2, &out1, &out2);
            config.is_redraw = false;
        }
        handle_event_display(&encoder);
        handle_action_display(&config, &encoder, &out1, &out2);
        vTaskDelay(pdMS_TO_TICKS(DELAY_DISPLAY_LOOP_MS));
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
            set_rtos_pin(GPIOA, GPIO_SPI1_NSS, arg_int);
            break;
        case U8X8_MSG_GPIO_DC:
            set_rtos_pin(OLED_DC_Port, OLED_DC_Pin, arg_int);
            break;
        case U8X8_MSG_GPIO_RESET:
            set_rtos_pin(OLED_RST_Port, OLED_RST_Pin, arg_int);
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
 * @param current_config
 * @param u8g2
 * @param out1
 * @param out2
 */
void draw_display(config_t *current_config, u8g2_t *u8g2, output_data_t *out1, output_data_t *out2) {
    switch (current_config->current_state) {
        case DISPLAY_INIT:
        case DISPLAY_MAIN:
            draw_main(current_config, u8g2, out1, out2);
            break;
        case DISPLAY_SET_CONFIG:
            draw_set_config(current_config, u8g2);
            break;
        case DISPLAY_TATTOO:
            draw_tattoo(current_config, u8g2);
            break;
        default:
            break;
    }
}

/**
 * @brief Draw main display
 *
 * @param current_config
 * @param u8g2
 * @param out1
 * @param out2
 */
void draw_main(config_t *current_config, u8g2_t *u8g2, output_data_t *out1, output_data_t *out2) {
    static char buffer[5];
    sprintf(buffer, "%dV", current_config->current_pdo);
    u8g2_FirstPage(u8g2);
    do {
        u8g2_SetFont(u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawFrame(u8g2, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        u8g2_DrawHLine(u8g2, 0, TOP_BOX_HEIGHT, DISPLAY_WIDTH);
        u8g2_DrawVLine(u8g2, DISPLAY_WIDTH / 2, TOP_BOX_HEIGHT, DISPLAY_HEIGHT - TOP_BOX_HEIGHT);
        u8g2_DrawStr(u8g2, 2, TOP_BOX_HEIGHT - 2, "Tattoo Machine");

        u8g2_DrawStr(u8g2, 106, TOP_BOX_HEIGHT - 2, buffer);

        draw_pane(u8g2, out1);
        draw_pane(u8g2, out2);
    } while (u8g2_NextPage(u8g2));
}

/**
 * @brief Draw the set config screen
 *
 * @param current_config
 * @param u8g2
 * @param out1
 * @param out2
 */
void draw_set_config(config_t *current_config, u8g2_t *u8g2) {
    static char buffer[20];
    sprintf(buffer, "Config %s", current_config->selected_output == OUT_1 ? "OUT 1" : "OUT 2");
    u8g2_FirstPage(u8g2);
    do {
        u8g2_SetFont(u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawFrame(u8g2, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        u8g2_DrawHLine(u8g2, 0, TOP_BOX_HEIGHT, DISPLAY_WIDTH);
        u8g2_DrawStr(u8g2, 32, TOP_BOX_HEIGHT - 2, buffer);

        sprintf(buffer, "%d.%dV", current_config->settings.voltage / 10, current_config->settings.voltage % 10);
        u8g2_DrawStr(u8g2, 4, 25, "Voltage :");
        draw_text(u8g2, 48, 25, current_config->cursor_idx == CONFIG_VOLTAGE, buffer);

        u8g2_DrawStr(u8g2, 75, 25, "Type :");
        draw_text(u8g2, 105, 25, current_config->cursor_idx == CONFIG_TYPE, current_config->settings.type == DC ? "DC" : "AC");

        draw_checkbox(u8g2, 15, 42, 10, current_config->settings.footswitch);
        draw_text(u8g2, 25, 41, current_config->cursor_idx == CONFIG_FOOTSWITCH, "Foot");

        draw_checkbox(u8g2, 70, 42, 10, current_config->settings.handswitch);
        draw_text(u8g2, 80, 41, current_config->cursor_idx == CONFIG_HANDSWITCH, "Hand");

        draw_button(u8g2, 20, 48, current_config->cursor_idx == CONFIG_SAVE, " Save ");
        draw_button(u8g2, 80, 48, current_config->cursor_idx == CONFIG_EXIT, " Exit ");
    } while (u8g2_NextPage(u8g2));
}

/**
 * @brief Draw the tattoo screen
 *
 * @param current_config
 * @param u8g2
 */
void draw_tattoo(config_t *current_config, u8g2_t *u8g2) {
    static char buffer[8];
    static uint16_t voltage_read = 0;
    sprintf(buffer, "%dV", current_config->current_pdo);
    u8g2_FirstPage(u8g2);
    do {
        u8g2_SetFont(u8g2, u8g2_font_helvR08_tr);
        u8g2_DrawFrame(u8g2, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        u8g2_DrawHLine(u8g2, 0, TOP_BOX_HEIGHT, DISPLAY_WIDTH);
        u8g2_DrawStr(u8g2, 2, TOP_BOX_HEIGHT - 2, "Tattoo Machine");
        u8g2_DrawStr(u8g2, 106, TOP_BOX_HEIGHT - 2, buffer);

        voltage_read = read_voltage(OUT_1);
        sprintf(buffer, "%d.%dV", voltage_read / 10, voltage_read % 10);
        u8g2_DrawStr(u8g2, 6, 40, buffer);

        voltage_read = read_voltage(OUT_2);
        sprintf(buffer, "%d.%dV", voltage_read / 10, voltage_read % 10);

        u8g2_DrawStr(u8g2, 128 - u8g2_GetStrWidth(u8g2, buffer) - 6, 40, buffer);

        u8g2_SetFont(u8g2, img_tattoo);
        u8g2_SetDrawColor(u8g2, 0);
        u8g2_DrawGlyph(u8g2, 45, 62, current_config->logo_blink ? 1001 : 1000);
        u8g2_SetDrawColor(u8g2, 1);
    } while (u8g2_NextPage(u8g2));
}

/**
 * @brief Handle the display state for actions
 *
 * @param current_config
 * @param encoder
 * @param out1
 * @param out2
 */
void handle_action_display(config_t *current_config, encoder_t *encoder, output_data_t *out1, output_data_t *out2) {
    switch (current_config->current_state) {
        case DISPLAY_MAIN:
            handle_action_main(current_config, encoder, out1, out2);
            break;
        case DISPLAY_SET_CONFIG:
            handle_action_set_config(current_config, encoder, out1, out2);
            break;
        case DISPLAY_TATTOO:
            handle_action_tattoo(current_config);
            break;
        default:
            break;
    }
}

/**
 * @brief Handle the main display state
 *
 * @param current_config
 * @param encoder
 * @param out1
 * @param out2
 */
void handle_action_main(config_t *current_config, encoder_t *encoder, output_data_t *out1, output_data_t *out2) {
    switch (encoder->event) {
        case EVENT_SELECT:
            if (out1->is_set_selected) {
                current_config->selected_output = OUT_1;
            } else {
                current_config->selected_output = OUT_2;
            }
            current_config->current_state = DISPLAY_SET_CONFIG;
            memcpy(&current_config->settings, current_config->selected_output == OUT_1 ? out1 : out2, sizeof(output_data_t));
            encoder->event = EVENT_NONE;
            current_config->is_redraw = true;
            break;
        case EVENT_NEXT:
        case EVENT_PREV:
            if (out1->is_set_selected) {
                out1->is_set_selected = false;
                out2->is_set_selected = true;
            } else {
                out1->is_set_selected = true;
                out2->is_set_selected = false;
            }
            encoder->event = EVENT_NONE;
            current_config->is_redraw = true;
            break;
    }
}

/**
 * @brief Handle action for set config
 *
 * @param current_config
 * @param encoder
 * @param out1
 * @param out2
 */
void handle_action_set_config(config_t *current_config, encoder_t *encoder, output_data_t *out1, output_data_t *out2) {
    switch (encoder->event) {
        case EVENT_SELECT:
            switch (current_config->cursor_idx) {
                case CONFIG_VOLTAGE:
                    current_config->selected = !current_config->selected;
                    encoder->event = EVENT_NONE;
                    current_config->is_redraw = true;
                    break;
                case CONFIG_TYPE:
                    current_config->settings.type = current_config->settings.type == DC ? PULSE : DC;
                    encoder->event = EVENT_NONE;
                    current_config->is_redraw = true;
                    break;
                case CONFIG_FOOTSWITCH:
                    current_config->settings.footswitch = !current_config->settings.footswitch;
                    encoder->event = EVENT_NONE;
                    current_config->is_redraw = true;
                    break;
                case CONFIG_HANDSWITCH:
                    current_config->settings.handswitch = !current_config->settings.handswitch;
                    encoder->event = EVENT_NONE;
                    current_config->is_redraw = true;
                    break;
                case CONFIG_SAVE:
                    memcpy(current_config->selected_output == OUT_1 ? out1 : out2, &current_config->settings, sizeof(output_data_t));
                    xQueueSend(output_config_queue, &current_config->settings, 0);
                    current_config->current_state = DISPLAY_MAIN;
                    current_config->cursor_idx = CONFIG_VOLTAGE;
                    encoder->event = EVENT_NONE;
                    current_config->is_redraw = true;
                    break;
                case CONFIG_EXIT:
                    current_config->current_state = DISPLAY_MAIN;
                    current_config->cursor_idx = CONFIG_VOLTAGE;
                    encoder->event = EVENT_NONE;
                    current_config->is_redraw = true;
                    break;
            }
            break;
        case EVENT_NEXT:
            if (current_config->selected) {
                if (current_config->settings.voltage < MAX_VOLTAGE) {
                    current_config->settings.voltage++;
                    current_config->is_redraw = true;
                }
            } else {
                if (current_config->cursor_idx < MAX_CONFIG_IDX) {
                    current_config->cursor_idx++;
                    current_config->is_redraw = true;
                }
            }
            encoder->event = EVENT_NONE;
            break;
        case EVENT_PREV:
            if (current_config->selected) {
                if (current_config->settings.voltage > MIN_VOLTAGE) {
                    current_config->settings.voltage--;
                    current_config->is_redraw = true;
                }
            } else {
                if (current_config->cursor_idx > 0) {
                    current_config->cursor_idx--;
                    current_config->is_redraw = true;
                }
            }
            encoder->event = EVENT_NONE;
            break;
    }
}

/**
 * @brief Handle action for tattoo display
 *
 */
void handle_action_tattoo(config_t *current_config) {
    if (current_config->logo_blink_counter++ >= BLINK_COUNTER) {
        current_config->logo_blink = !current_config->logo_blink;
        current_config->is_redraw = true;
        current_config->logo_blink_counter = 0;
    }
}

void draw_text(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t is_selected, const char *text) {
    uint8_t w = u8g2_GetStrWidth(u8g2, text);
    uint8_t h = u8g2_GetMaxCharHeight(u8g2);
    if (is_selected) {
        u8g2_SetDrawColor(u8g2, 1);
        u8g2_DrawBox(u8g2, x, y - h + 1, w, h);
        u8g2_SetDrawColor(u8g2, 0);
    }
    u8g2_DrawStr(u8g2, x, y, text);
    if (is_selected) {
        u8g2_SetDrawColor(u8g2, 1);
    }
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
 * @brief Draw a checkbox
 *
 * @param u8g2
 * @param x
 * @param y
 * @param w
 * @param is_checked
 */
void draw_checkbox(u8g2_t *u8g2, uint8_t x, uint8_t y, uint8_t w, uint8_t is_checked) {
    u8g2_DrawFrame(u8g2, x, y - w, w, w);
    if (is_checked) {
        w -= 4;
        u8g2_DrawBox(u8g2, x + 2, y - w - 2, w, w);
    }
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

/**
 * @brief Read adc and convert to voltage
 *
 * @param output
 * @return uint16_t
 */
uint16_t read_voltage(output_t output) {
    uint16_t raw = 0;
    float voltage = 0;
    switch (output) {
        case OUT_1:
            raw = read_adc_native(1);
            break;
        case OUT_2:
            raw = read_adc_native(0);
            break;
    }
    voltage = (float)raw / 4095 * 3.3;
    voltage = voltage * (R1_Analog + R2_Analog) / R2_Analog;
    voltage = voltage / 0.1;
    return (uint16_t)voltage;
}