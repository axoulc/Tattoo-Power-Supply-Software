#include "ee.h"
#include "memory.h"

bool eeprom_init(void) {
    uint8_t buffer[2] = { 0 };
    if (!ee_init()) {
        return false;
    }
    if (ee_read(BASE_ADRESS_OUT1, 1, &buffer[0]) && ee_read(BASE_ADRESS_OUT2, 1, &buffer[1])) {
        if (buffer[0] == IDENT_OUT1 && buffer[1] == IDENT_OUT2)
            return true;
    }
    return ee_format(false);
}

bool eeprom_read(output_config_t * data) {
    uint8_t buffer = 0;
    if (ee_read(data->output == OUT_1 ? BASE_ADRESS_OUT1 : BASE_ADRESS_OUT2, 1, &buffer)) {
        if (buffer == (data->output == OUT_1 ? IDENT_OUT1 : IDENT_OUT2)) {
            return ee_read(data->output == OUT_1 ? BASE_ADRESS_OUT1 + 1 : BASE_ADRESS_OUT2 + 1, sizeof(output_config_t), (uint8_t *) data);
        } else {
            return false;
        }
    }
    return false;
}

bool eeprom_write(output_config_t * data) {
    uint8_t buffer[1 + sizeof(output_config_t)] = { 0 };
    buffer[0] = data->output == OUT_1 ? IDENT_OUT1 : IDENT_OUT2;
    memcpy(&buffer[1], data, sizeof(output_config_t));
    return ee_write(data->output == OUT_1 ? BASE_ADRESS_OUT1 : BASE_ADRESS_OUT2, 1 + sizeof(output_config_t), buffer);
}