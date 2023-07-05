#ifndef TATTOO_TYPES_H
#define TATTOO_TYPES_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum {
    OUT_1,
    OUT_2
} output_t;

typedef enum {
    DC,
    PULSE
} supply_type_t;

typedef struct __attribute__((__packed__)) {
    output_t output;
    uint16_t voltage;
    supply_type_t type;
    bool footswitch;
    bool handswitch;
} output_config_t;

typedef enum {
    POWER_OFF,
    POWER_ON_FOOT,
    POWER_ON_HAND
} power_state_t;

#endif