#ifndef PERIPHERALS_H
#define PERIPHERALS_H

void configure_clock(void);
void configure_gpio(void);
void configure_spi(void);
void configure_usart(void);
int _write(int file, char *ptr, int len);
void configure_i2c(void);
void configure_adc(void);
uint16_t read_adc_native(uint8_t channel);

#endif