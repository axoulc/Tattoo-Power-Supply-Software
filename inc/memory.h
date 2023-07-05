#ifndef MEMORY_H
#define MEMORY_H

#include "tattoo_types.h"

#define OUT_SIZE            (1 + sizeof(output_config_t) + ((1 + sizeof(output_config_t)) % 4))
#define BASE_ADRESS_OUT1    0
#define BASE_ADRESS_OUT2    (BASE_ADRESS_OUT1 + OUT_SIZE)
#define IDENT_OUT1          0xDD
#define IDENT_OUT2          0xEE

bool eeprom_init(void);
bool eeprom_read(output_config_t * data);
bool eeprom_write(output_config_t * data);

#endif