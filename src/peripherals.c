#include <errno.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <stdio.h>

#include "peripherals.h"

void configure_clock(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    //rcc_set_usbpre(RCC_CFGR_USBPRE_PLL_CLK_DIV1_5);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_I2C1);
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_TIM1);
}

void configure_gpio(void) {
    // PA8: SW1
    // PA9: SW2
    // PA15: USB_DFU
    gpio_set_mode(SW1_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, SW1_Pin);
    gpio_set_mode(SW2_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, SW2_Pin);
    gpio_set_mode(USB_DFU_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, USB_DFU_Pin);
    gpio_clear(SW1_Port, SW1_Pin);
    gpio_clear(SW2_Port, SW2_Pin);

    // PB0: EN_SMPS
    gpio_set_mode(EN_SMPS_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, EN_SMPS_Pin);
    gpio_clear(EN_SMPS_Port, EN_SMPS_Pin);
}

void configure_encoder(void) {
    // PA8 : DT
    // PA9 : CLK
    // PA10: SW
    gpio_set_mode(Encod_DT_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, Encod_DT_Pin);
    gpio_set_mode(Encod_CLK_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, Encod_CLK_Pin);
    gpio_set_mode(Encod_SW_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, Encod_SW_Pin);

    // Timer 1
    timer_set_period(TIM1, 0xFFFF);
    timer_slave_set_mode(TIM1, TIM_SMCR_SMS_EM3); // Encoder Mode 3
    timer_ic_set_input(TIM1, TIM_IC1, TIM_IC_IN_TI1);
    timer_ic_set_input(TIM1, TIM_IC2, TIM_IC_IN_TI2);
    timer_enable_counter(TIM1);

    //int motor_pos = timer_get_count(TIM3);
}

void configure_spi(void) {
    // SPI1
    // /CS: PA4
    // SCK: PA5
    // MISO: PA6
    // MOSI: PA7
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_SPI1_NSS | GPIO_SPI1_SCK | GPIO_SPI1_MOSI);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_SPI1_MISO);

    rcc_periph_reset_pulse(RST_SPI1);
    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);
    spi_enable(SPI1);
}

void configure_usart(void) {
    // USART2
    // TX: PA2
    // RX: PA3
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);

    /* Setup UART parameters. */
    usart_set_baudrate(USART2, 115200);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_mode(USART2, USART_MODE_TX);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_enable(USART2);
}

int _write(int file, char *ptr, int len) {
    int i;

    if (file == 1) {
        for (i = 0; i < len; i++)
            usart_send_blocking(USART2, ptr[i]);
        return i;
    }

    errno = EIO;
    return -1;
}

void configure_i2c(void) {
    // I2C1
    // SCL: PB6
    // SDA: PB7
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C1_SCL | GPIO_I2C1_SDA);

    /* Setup I2C parameters. */
    i2c_peripheral_disable(I2C1);
    i2c_set_clock_frequency(I2C1, 36);
    i2c_set_fast_mode(I2C1);
    i2c_set_ccr(I2C1, 0x1e);
    i2c_set_trise(I2C1, 0x0b);
    i2c_peripheral_enable(I2C1);
}

void configure_adc(void) {
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO0);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO1);

    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_239DOT5CYC);
    adc_power_on(ADC1);

    int i;
    for (i = 0; i < 800000; i++)
        __asm__("nop");

    adc_reset_calibration(ADC1);
    adc_calibrate(ADC1);
}

uint16_t read_adc_native(uint8_t channel) {
    uint8_t channel_array[16];
    channel_array[0] = channel;
    adc_set_regular_sequence(ADC1, 1, channel_array);
    adc_start_conversion_direct(ADC1);
    while (!adc_eoc(ADC1))
        ;
    uint16_t reg16 = adc_read_regular(ADC1);
    return reg16;
}