#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "tattoo_types.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define TOP_BOX_HEIGHT 12
#define MAX_CONFIG_IDX 5

#define MAX_VOLTAGE 150
#define MIN_VOLTAGE 10

#define DELAY_DISPLAY_LOOP_MS 25
#define BLINK_DELAY 500
#define BLINK_COUNTER (BLINK_DELAY / DELAY_DISPLAY_LOOP_MS)

typedef struct {
    output_t output;
    uint16_t voltage;
    supply_type_t type;
    bool footswitch;
    bool handswitch;

    bool is_set_selected;
    uint16_t x_offset;
} output_data_t;

typedef enum {
    EVENT_NONE,
    EVENT_SELECT,
    EVENT_NEXT,
    EVENT_PREV
} event_t;

typedef struct {
    uint32_t current;
    uint32_t last;
    event_t event;
} encoder_t;

typedef enum {
    DISPLAY_INIT,
    DISPLAY_MAIN,
    DISPLAY_SET_CONFIG,
    DISPLAY_TATTOO
} display_state_t;

typedef enum {
    CONFIG_VOLTAGE = 0,
    CONFIG_TYPE,
    CONFIG_FOOTSWITCH,
    CONFIG_HANDSWITCH,
    CONFIG_SAVE,
    CONFIG_EXIT
} config_idx_t;

typedef struct {
    uint8_t current_pdo;
    display_state_t current_state;
    bool is_redraw;
    bool selected;
    output_t selected_output;
    config_idx_t cursor_idx;
    output_data_t settings;
    bool logo_blink;
    uint8_t logo_blink_counter;
} config_t;

void display_task(void *pvParameters);

#endif