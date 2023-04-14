#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define TOP_BOX_HEIGHT 12

typedef enum {
    OUT_1,
    OUT_2
} output_t;

typedef struct {
    output_t output;
    uint16_t voltage;
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
    DISPLAY_SET_CONFIG
} display_state_t;

void display_init_task(void);

#endif