#ifndef STUSB4500_H
#define STUSB4500_H

#include <stdint.h>
#include <stdbool.h>
#include "stusb4500_register_map.h"

typedef uint8_t (*stusb4500_i2c_write)(uint8_t addr, uint16_t reg, uint8_t *data_w, uint16_t len);
typedef uint8_t (*stusb4500_i2c_read)(uint8_t addr, uint16_t reg, uint8_t *data_r, uint16_t len);
typedef void (*stusb4500_hard_reset_high)(void);
typedef void (*stusb4500_hard_reset_low)(void);
typedef void (*stusb4500_delay_ms)(uint16_t ms);

typedef struct {
    stusb4500_i2c_write i2c_write;
    stusb4500_i2c_read i2c_read;
    stusb4500_hard_reset_high hard_reset_high;
    stusb4500_hard_reset_low hard_reset_low;
    stusb4500_delay_ms delay_ms;

    uint8_t addr;

    uint8_t sector[5][8];
    uint8_t read_sector;

    bool hard_reset;
} stusb4500_t;


void stusb4500_begin(stusb4500_t *stusb4500);
void stusb4500_read(stusb4500_t *stusb4500);
void stusb4500_write(stusb4500_t *stusb4500, uint8_t default_values);

float stusb4500_get_voltage(stusb4500_t *stusb4500, uint8_t pdo_num);
float stusb4500_get_current(stusb4500_t *stusb4500, uint8_t pdo_num);
uint8_t stusb4500_get_upper_voltage_limit(stusb4500_t *stusb4500, uint8_t pdo_num);
uint8_t stusb4500_get_lower_voltage_limit(stusb4500_t *stusb4500, uint8_t pdo_num);
float stusb4500_get_flex_current(stusb4500_t *stusb4500);
uint8_t stusb4500_get_PDO_number(stusb4500_t *stusb4500);
uint8_t stusb4500_get_external_power(stusb4500_t *stusb4500);
uint8_t stusb4500_get_usb_comm_capable(stusb4500_t *stusb4500);
uint8_t stusb4500_get_config_ok_gpio(stusb4500_t *stusb4500);
uint8_t stusb4500_get_gpio_ctrl(stusb4500_t *stusb4500);
uint8_t stusb4500_get_power_above_5v(stusb4500_t *stusb4500);
uint8_t stusb4500_get_req_src_current(stusb4500_t *stusb4500);


void stusb4500_set_voltage(stusb4500_t *stusb4500, uint8_t pdo_num, float voltage);
void stusb4500_set_current(stusb4500_t *stusb4500, uint8_t pdo_num, float current);
void stusb4500_set_upper_voltage_limit(stusb4500_t *stusb4500, uint8_t pdo_num, uint8_t limit);
void stusb4500_set_lower_voltage_limit(stusb4500_t *stusb4500, uint8_t pdo_num, uint8_t limit);
void stusb4500_set_flex_current(stusb4500_t *stusb4500, float current);
void stusb4500_set_PDO_number(stusb4500_t *stusb4500, uint8_t pdo_num);
void stusb4500_set_external_power(stusb4500_t *stusb4500, uint8_t external_power);
void stusb4500_set_usb_comm_capable(stusb4500_t *stusb4500, uint8_t usb_comm_capable);
void stusb4500_set_config_ok_gpio(stusb4500_t *stusb4500, uint8_t config_ok_gpio);
void stusb4500_set_gpio_ctrl(stusb4500_t *stusb4500, uint8_t gpio_ctrl);
void stusb4500_set_power_above_5v(stusb4500_t *stusb4500, uint8_t power_above_5v);
void stusb4500_set_req_src_current(stusb4500_t *stusb4500, uint8_t req_src_current);

void stusb4500_reset(stusb4500_t *stusb4500);

#endif