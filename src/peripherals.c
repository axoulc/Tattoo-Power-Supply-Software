#include "peripherals.h"

#include <errno.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"

void configure_clock(void);
void configure_gpio(void);
void configure_encoder(void);
void configure_spi(void);
void configure_usart(void);
void configure_i2c(void);
void configure_adc(void);

extern SemaphoreHandle_t gpiob_sem_handle;
extern SemaphoreHandle_t i2c_sem_handle;

void configure_mcu(void) {
    configure_clock();
    configure_gpio();
    configure_encoder();
    configure_spi();
    configure_usart();
    configure_i2c();
    configure_adc();
}

void configure_clock(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    // rcc_set_usbpre(RCC_CFGR_USBPRE_PLL_CLK_DIV1_5);
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
    // PB0: EN_SMPS
    // PB8: OLED_DC
    // PB9: OLED_RST
    // PB12: LED
    gpio_set_mode(EN_SMPS_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, EN_SMPS_Pin);
    gpio_set_mode(LED_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_Pin);
    gpio_set_mode(OLED_DC_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, OLED_DC_Pin);
    gpio_set_mode(OLED_RST_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, OLED_RST_Pin);
    
    gpio_set(EN_SMPS_Port, EN_SMPS_Pin);
    gpio_clear(LED_Port, LED_Pin);
    gpio_clear(OLED_DC_Port, OLED_DC_Pin);
    gpio_clear(OLED_RST_Port, OLED_RST_Pin);

    // PB13: SW1
    // PB14: SW2
    // PA15: USB_DFU
    gpio_set_mode(SW1_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, SW1_Pin);
    gpio_set_mode(SW2_Port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, SW2_Pin);
    gpio_set_mode(USB_DFU_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, USB_DFU_Pin);
    gpio_clear(SW1_Port, SW1_Pin);
    gpio_clear(SW2_Port, SW2_Pin);

    // PB10: Footswitch
    // PB11: Handswitch
    gpio_set_mode(Footswitch_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, Footswitch_Pin);
    gpio_set_mode(Handswitch_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, Handswitch_Pin);
}

uint16_t get_footswitch_state(void) {
    return gpio_get(Footswitch_Port, Footswitch_Pin);
}

uint16_t get_handswitch_state(void) {
    return gpio_get(Handswitch_Port, Handswitch_Pin);
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
    timer_slave_set_mode(TIM1, TIM_SMCR_SMS_EM3);  // Encoder Mode 3
    timer_ic_set_input(TIM1, TIM_IC1, TIM_IC_IN_TI1);
    timer_ic_set_input(TIM1, TIM_IC2, TIM_IC_IN_TI2);
    timer_ic_set_filter(TIM1, TIM_IC1, TIM_IC_CK_INT_N_8);
    timer_ic_set_filter(TIM1, TIM_IC2, TIM_IC_CK_INT_N_8);
    timer_enable_counter(TIM1);
}

uint32_t get_encoder_count(void) {
    return timer_get_counter(TIM1);
}

uint16_t get_encoder_rot(void) {
    return gpio_get(Encod_CLK_Port, Encod_CLK_Pin) && gpio_get(Encod_DT_Port, Encod_DT_Pin);
}

uint16_t get_encoder_sw(void) {
    return gpio_get(Encod_SW_Port, Encod_SW_Pin);
}

void configure_spi(void) {
    // SPI1
    // /CS: PA4
    // SCK: PA5
    // MISO: PA6
    // MOSI: PA7
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_SPI1_NSS | GPIO_SPI1_SCK | GPIO_SPI1_MOSI);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_SPI1_MISO);
    gpio_set(GPIOA, GPIO_SPI1_NSS);

    rcc_periph_reset_pulse(RST_SPI1);
    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
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
    i2c_set_speed(I2C1, i2c_speed_sm_100k, 36);
    i2c_peripheral_enable(I2C1);
}

uint8_t write_i2c(uint8_t addr, uint16_t reg, uint8_t *data_w, uint16_t len) {
    uint8_t tx[3] = { 0 };
    tx[0] = reg;
    for (int i = 0; i < len; i++) {
        tx[i + 1] = data_w[i];
    }
    xSemaphoreTake(i2c_sem_handle, portMAX_DELAY);
    i2c_transfer7(I2C1, addr, tx, len + 1, NULL, 0);
    xSemaphoreGive(i2c_sem_handle);
    return 0;
}

uint8_t read_i2c(uint8_t addr, uint16_t reg, uint8_t *data_r, uint16_t len) {
    uint8_t tx = reg;
    xSemaphoreTake(i2c_sem_handle, portMAX_DELAY);
    i2c_transfer7(I2C1, addr, &tx, 1, data_r, len);
    xSemaphoreGive(i2c_sem_handle);
    return 0;
}

void configure_adc(void) {
    // PA0: ADC1_IN0
    // PA1: ADC1_IN1
    gpio_set_mode(Analog1_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, Analog1_Pin);
    gpio_set_mode(Analog2_Port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, Analog2_Pin);

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

void spi_send_byte(uint8_t byte) {
    spi_send(SPI1, (uint8_t) byte);
}

void set_rtos_pin(uint32_t gpioport, uint16_t gpios, uint8_t state) {
    if (gpioport == GPIOB)
        xSemaphoreTake(gpiob_sem_handle, portMAX_DELAY);

    if (state)
        gpio_set(gpioport, gpios);
    else
        gpio_clear(gpioport, gpios);

    if (gpioport == GPIOB)
        xSemaphoreGive(gpiob_sem_handle);
}

