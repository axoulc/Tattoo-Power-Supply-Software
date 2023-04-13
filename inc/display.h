#ifndef DISPLAY_H
#define DISPLAY_H

typedef struct {
    uint32_t current = 0;
    uint32_t last = 1;
} encoder_t;

void display_init_task(void);

#endif