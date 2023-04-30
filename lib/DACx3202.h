#ifndef DACX3202_H
#define DACX3202_H

#include <stdint.h>

#define DACX3202_7B_ADDR(DACX3202_A0) ( 0x48 | (DACX3202_A0 & 0x03) )

#define DAC63202_DEVICE_ID                  0x06
#define DAC53202_DEVICE_ID                  0x07

#define DACX3202_REG_NOP                    0x00
#define DACX3202_REG_DAC_1_MARGIN_HIGH      0x01
#define DACX3202_REG_DAC_1_MARGIN_LOW       0x02
#define DACX3202_REG_DAC_1_VOUT_CMP_CONFIG  0x03
#define DACX3202_REG_DAC_1_IOUT_MISC_CONFIG 0x04
#define DACX3202_REG_DAC_1_CMP_MODE_CONFIG  0x05
#define DACX3202_REG_DAC_1_FUNC_CONFIG      0x06
#define DACX3202_REG_DAC_0_MARGIN_HIGH      0x13
#define DACX3202_REG_DAC_0_MARGIN_LOW       0x14
#define DACX3202_REG_DAC_0_VOUT_CMP_CONFIG  0x15
#define DACX3202_REG_DAC_0_IOUT_MISC_CONFIG 0x16
#define DACX3202_REG_DAC_0_CMP_MODE_CONFIG  0x17
#define DACX3202_REG_DAC_0_FUNC_CONFIG      0x18
#define DACX3202_REG_DAC_1_DATA             0x19
#define DACX3202_REG_DAC_0_DATA             0x1C
#define DACX3202_REG_COMMON_CONFIG          0x1F
#define DACX3202_REG_COMMON_TRIGGER         0x20
#define DACX3202_REG_COMMON_DAC_TRIG        0x21
#define DACX3202_REG_GENERAL_STATUS         0x22
#define DACX3202_REG_CMP_STATUS             0x23
#define DACX3202_REG_GPIO_CONFIG            0x24
#define DACX3202_REG_DEVICE_MODE_CONFIG     0x25
#define DACX3202_REG_INTERFACE_CONFIG       0x26
#define DACX3202_REG_SRAM_CONFIG            0x2B
#define DACX3202_REG_SRAM_DATA              0x2C
#define DACX3202_REG_BROADCAST_CONFIG       0x50

#define DACX3202_VOLTAGE_TO_DATA(voltage, vref, type) (uint16_t)((voltage / vref) * (1 << (type == DAC53202_10b ? 10 : 12)))

typedef uint8_t (*dacx3202_i2c_write)(uint8_t addr, uint16_t reg, uint8_t *data_w, uint16_t len);
typedef uint8_t (*dacx3202_i2c_read)(uint8_t addr, uint16_t reg, uint8_t *data_r, uint16_t len);

typedef enum {
    DAC53202_10b = 0,
    DAC63202_12b = 1
} dacx3202_type_t;

typedef enum {
    DACX3202_DAC_0 = 0,
    DACX3202_DAC_1 = 1
} dacx3202_dac_t;

typedef struct {
    dacx3202_i2c_write i2c_write;
    dacx3202_i2c_read i2c_read;

    dacx3202_type_t type;
    uint8_t addr;
    float vref;
} dacx3202_t;


uint8_t dacx3202_init(dacx3202_t *dacx3202);
void dacx3202_power_up(dacx3202_t *dacx3202, dacx3202_dac_t channel);
void dacx3202_set_value(dacx3202_t *dacx3202, dacx3202_dac_t channel, uint16_t value);
void dacx3202_set_voltage(dacx3202_t *dacx3202, dacx3202_dac_t channel, float voltage);

#endif