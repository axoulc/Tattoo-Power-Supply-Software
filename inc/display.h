#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define TOP_BOX_HEIGHT 12

typedef struct {
    uint32_t current;
    uint32_t last;
} encoder_t;

void display_init_task(void);

#endif