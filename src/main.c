#include <stdint.h>
#include <stdio.h>

int main(void) {
	configure_clock();
	configure_gpio();
	configure_spi();
	configure_usart();
	configure_i2c();
	configure_adc();
	
	while (1) {
		__asm__("nop");
	}

	return 0;
}