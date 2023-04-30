#include "power.h"

#include "FreeRTOS.h"
#include "peripherals.h"
#include "task.h"
#include "DACx3202.h"

void power_task(void *pvParameters) {
    (void)pvParameters;

    dacx3202_t dacx3202 = {
        .addr = DACX3202_7B_ADDR(0x00),
        .i2c_read = read_i2c,
        .i2c_write = write_i2c,
        .vref = 3.3
    };

    if (dacx3202_init(&dacx3202) != 0) {
        // TODO: Handle error
    }

    for (;;) {
        
    }
    
}    