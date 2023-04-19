#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define TOP_BOX_HEIGHT 12
#define MAX_CONFIG_IDX 5

#define MAX_VOLTAGE 150
#define MIN_VOLTAGE 10

typedef enum {
    OUT_1,
    OUT_2
} output_t;

typedef enum {
    DC,
    PULSE
} supply_type_t;

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
    bool logo_blink;
    output_t selected_output;
    config_idx_t cursor_idx;
    output_data_t settings;
} config_t;

void display_init_task(void);

#endif