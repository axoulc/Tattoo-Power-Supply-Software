#include "DACx3202.h"

uint8_t dacx3202_init(dacx3202_t *dacx3202) {
    uint8_t buffer[2] = { 0 };
    uint16_t reg_data = 0;
    dacx3202->i2c_read(dacx3202->addr, DACX3202_REG_GENERAL_STATUS, buffer, 2);
    reg_data = (buffer[0] << 8) | buffer[1];
    if (reg_data == 0x00) {
        return 1;
    }
    if (((reg_data >> 2) & 0x3f) == DAC63202_DEVICE_ID) {
        dacx3202->type = DAC63202_12b;
    } else if (((reg_data >> 2) & 0x3f) == DAC53202_DEVICE_ID) {
        dacx3202->type = DAC53202_10b;
    } else {
        return 1;
    }
    return 0;
}

void dacx3202_power_up(dacx3202_t *dacx3202, dacx3202_dac_t channel) {
    uint8_t buffer[2] = { 0 };
    uint16_t reg_data = 0;
    
    dacx3202->i2c_read(dacx3202->addr, DACX3202_REG_COMMON_CONFIG, buffer, 2);
    reg_data = (buffer[0] << 8) | buffer[1];
    reg_data &= ~(3 << (channel == DACX3202_DAC_0 ? 10 : 1));
    buffer[0] = (reg_data >> 8) & 0xff;
    buffer[1] = reg_data & 0xff;
    dacx3202->i2c_write(dacx3202->addr, DACX3202_REG_COMMON_CONFIG, buffer, 2);
}

void dacx3202_set_value(dacx3202_t *dacx3202, dacx3202_dac_t channel, uint16_t value) {
    uint8_t buffer[2] = { 0 };
    value = dacx3202->type == DAC63202_12b ? value << 4 : value << 6;
    buffer[0] = (value >> 8) & 0xff;
    buffer[1] = value & 0xff;
    dacx3202->i2c_write(dacx3202->addr, channel == DACX3202_DAC_0 ? DACX3202_REG_DAC_0_DATA : DACX3202_REG_DAC_1_DATA, buffer, 2);
}

void dacx3202_set_voltage(dacx3202_t *dacx3202, dacx3202_dac_t channel, float voltage) {
    uint8_t buffer[2] = { 0 };
    uint16_t dac_value = DACX3202_VOLTAGE_TO_DATA(voltage, dacx3202->vref, dacx3202->type);
    dac_value = dacx3202->type == DAC63202_12b ? dac_value << 4 : dac_value << 6;
    buffer[0] = (dac_value >> 8) & 0xff;
    buffer[1] = dac_value & 0xff;
    dacx3202->i2c_write(dacx3202->addr, channel == DACX3202_DAC_0 ? DACX3202_REG_DAC_0_DATA : DACX3202_REG_DAC_1_DATA, buffer, 2);
}