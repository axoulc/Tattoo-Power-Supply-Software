#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <libopencm3/stm32/gpio.h>

// Analog input channels
#define Analog1_Port    GPIOA
#define Analog1_Pin     GPIO0
#define Analog2_Port    GPIOA
#define Analog2_Pin     GPIO1

// Digital input channels
#define USB_DFU_Port    GPIOA
#define USB_DFU_Pin     GPIO15

// Digital output channels
#define EN_SMPS_Port    GPIOB
#define EN_SMPS_Pin     GPIO0
#define SW1_Port        GPIOB
#define SW1_Pin         GPIO13
#define SW2_Port        GPIOB
#define SW2_Pin         GPIO14

// Encoder channels
#define Encod_DT_Port   GPIO_BANK_TIM1_CH1
#define Encod_DT_Pin    GPIO_TIM1_CH1
#define Encod_CLK_Port  GPIO_BANK_TIM1_CH1
#define Encod_CLK_Pin   GPIO_TIM1_CH2
#define Encod_SW_Port   GPIOA
#define Encod_SW_Pin    GPIO10

// OLED GPIO
#define OLED_DC_Port    GPIOB
#define OLED_DC_Pin     GPIO8
#define OLED_RST_Port   GPIOB
#define OLED_RST_Pin    GPIO9

// Switch & LED GPIO
#define Footswitch_Port GPIOB
#define Footswitch_Pin  GPIO10
#define Handswitch_Port GPIOB
#define Handswitch_Pin  GPIO11
#define LED_Port        GPIOB
#define LED_Pin         GPIO12

void configure_clock(void);
void configure_gpio(void);
void configure_encoder(void);
uint32_t get_encoder_count(void);
uint16_t get_encoder_rot(void);
uint16_t get_encoder_sw(void);
void configure_spi(void);
void configure_usart(void);
int _write(int file, char *ptr, int len);
void configure_i2c(void);
void configure_adc(void);
uint16_t read_adc_native(uint8_t channel);
void spi_send_byte(uint8_t byte);
void set_cs_pin(uint8_t state);
void set_dc_pin(uint8_t state);
void set_rst_pin(uint8_t state);

#endif