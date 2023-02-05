#include "stusb4500.h"

uint32_t stusb4500_read_PDO(stusb4500_t *stusb4500, uint8_t pdo_num);
void stusb4500_write_PDO(stusb4500_t *stusb4500, uint8_t pdo_num, uint32_t pdoData);
uint8_t CUST_EnterWriteMode(stusb4500_t *stusb4500, unsigned char ErasedSector);
uint8_t CUST_ExitTestMode(stusb4500_t *stusb4500);
uint8_t CUST_WriteSector(stusb4500_t * stusb4500, char SectorNum, unsigned char *SectorData);

/**
 * @brief Initialize the STUSB4500
 * @param stusb4500 Pointer to the STUSB4500 object
 * @return uint8_t 0 on success, 1 on failure
 */
void stusb4500_begin(stusb4500_t *stusb4500) {
    // TODO - Initialize the STUSB4500 Better way
    stusb4500->read_sector = 0;
    stusb4500_read(stusb4500);
}

/**
 * @brief Read the STUSB4500 registers
 *
 * @param stusb4500 Pointer to the STUSB4500 object
 */
void stusb4500_read(stusb4500_t *stusb4500) {
    uint8_t buffer;
    stusb4500->read_sector = 1;

    buffer = FTP_CUST_PASSWORD; /* Set Password 0x95->0x47*/

    stusb4500->i2c_write(stusb4500->addr, FTP_CUST_PASSWORD_REG, &buffer, 1);
    stusb4500->i2c_write(stusb4500->addr, FTP_CUST_PASSWORD_REG, &buffer, 1);

    buffer = 0; /* NVM internal controller reset 0x96->0x00*/
    stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1);

    buffer = FTP_CUST_PWR | FTP_CUST_RST_N; /* Set PWR and RST_N bits 0x96->0xC0*/
    stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1);

    //--- End of CUST_EnterReadMode

    for (uint8_t i = 0; i < 5; i++) {
        buffer = FTP_CUST_PWR | FTP_CUST_RST_N; /* Set PWR and RST_N bits 0x96->0xC0*/
        stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1);

        buffer = (READ & FTP_CUST_OPCODE); /* Set Read Sectors Opcode 0x97->0x00*/
        stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_1, &buffer, 1);

        buffer = (i & FTP_CUST_SECT) | FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
        stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1); /* Load Read Sectors Opcode */

        do {
            stusb4500->i2c_read(stusb4500->addr, FTP_CTRL_0, &buffer, 1); /* Wait for execution */
        } while (buffer & FTP_CUST_REQ);                                  // The FTP_CUST_REQ is cleared by NVM controller when the operation is finished.

        stusb4500->i2c_read(stusb4500->addr, buffer, &stusb4500->sector[i][0], 8);
    }

    CUST_ExitTestMode(stusb4500);

    // NVM settings get loaded into the volatile registers after a hard reset or power cycle.
    // Below we will copy over some of the saved NVM settings to the I2C registers
    uint8_t currentValue;

    // PDO Number
    stusb4500_set_PDO_number(stusb4500, (stusb4500->sector[3][2] & 0x06) >> 1);

    // PDO1 - fixed at 5V and is unable to change
    stusb4500_set_voltage(stusb4500, 1, 5.0);

    currentValue = (stusb4500->sector[3][2] & 0xF0) >> 4;
    if (currentValue == 0)
        stusb4500_set_current(stusb4500, 1, 0);
    else if (currentValue < 11)
        stusb4500_set_current(stusb4500, 1, currentValue * 0.25 + 0.25);
    else
        stusb4500_set_current(stusb4500, 1, currentValue * 0.50 - 2.50);

    // PDO2
    stusb4500_set_voltage(stusb4500, 2, ((stusb4500->sector[4][1] << 2) + (stusb4500->sector[4][0] >> 6)) / 20.0);

    currentValue = (stusb4500->sector[3][4] & 0x0F);
    if (currentValue == 0)
        stusb4500_set_current(stusb4500, 2, 0);
    else if (currentValue < 11)
        stusb4500_set_current(stusb4500, 2, currentValue * 0.25 + 0.25);
    else
        stusb4500_set_current(stusb4500, 2, currentValue * 0.50 - 2.50);

    // PDO3
    stusb4500_set_voltage(stusb4500, 3, (((stusb4500->sector[4][3] & 0x03) << 8) + stusb4500->sector[4][2]) / 20.0);

    currentValue = (stusb4500->sector[3][5] & 0xF0) >> 4;
    if (currentValue == 0)
        stusb4500_set_current(stusb4500, 3, 0);
    else if (currentValue < 11)
        stusb4500_set_current(stusb4500, 3, currentValue * 0.25 + 0.25);
    else
        stusb4500_set_current(stusb4500, 3, currentValue * 0.50 - 2.50);
}

/**
 * @brief Write the STUSB4500 registers
 *
 * @param stusb4500 Pointer to stusb4500_t struct
 * @param default_values 0 = use values from NVM, 1 = use default values
 */
void stusb4500_write(stusb4500_t *stusb4500, uint8_t default_values) {
    if (default_values == 0) {
        uint8_t nvmCurrent[] = {0, 0, 0};
        float voltage[] = {0, 0, 0};

        uint32_t digitalVoltage = 0;

        // Load current values into NVM
        for (uint8_t i = 0; i < 3; i++) {
            uint32_t pdoData = stusb4500_read_PDO(stusb4500, i + 1);
            float current = (pdoData & 0x3FF) * 0.01;  // The current is the first 10-bits of the 32-bit PDO register (10mA resolution)

            if (current > 5.0) current = 5.0;  // Constrain current value to 5A max

            /*Convert current from float to 4-bit value
         -current from 0.5-3.0A is set in 0.25A steps
         -current from 3.0-5.0A is set in 0.50A steps
        */
            if (current < 0.5)
                nvmCurrent[i] = 0;
            else if (current <= 3)
                nvmCurrent[i] = (4 * current) - 1;
            else
                nvmCurrent[i] = (2 * current) + 5;

            digitalVoltage = (pdoData >> 10) & 0x3FF;  // The voltage is bits 10:19 of the 32-bit PDO register
            voltage[i] = digitalVoltage / 20.0;        // Voltage has 50mV resolution

            // Make sure the minimum voltage is between 5-20V
            if (voltage[i] < 5.0)
                voltage[i] = 5.0;
            else if (voltage[i] > 20.0)
                voltage[i] = 20.0;
        }

        // load current for PDO1 (sector 3, byte 2, bits 4:7)
        stusb4500->sector[3][2] &= 0x0F;                  // clear bits 4:7
        stusb4500->sector[3][2] |= (nvmCurrent[0] << 4);  // load new amperage for PDO1

        // load current for PDO2 (sector 3, byte 4, bits 0:3)
        stusb4500->sector[3][4] &= 0xF0;           // clear bits 0:3
        stusb4500->sector[3][4] |= nvmCurrent[1];  // load new amperage for PDO2

        // load current for PDO3 (sector 3, byte 5, bits 4:7)
        stusb4500->sector[3][5] &= 0x0F;                  // clear bits 4:7
        stusb4500->sector[3][5] |= (nvmCurrent[2] << 4);  // set amperage for PDO3

        // The voltage for PDO1 is 5V and cannot be changed

        // PDO2
        // Load voltage (10-bit)
        // -bit 9:2 - sector 4, byte 1, bits 0:7
        // -bit 0:1 - sector 4, byte 0, bits 6:7
        digitalVoltage = voltage[1] * 20;                           // convert votlage to 10-bit value
        stusb4500->sector[4][0] &= 0x3F;                            // clear bits 6:7
        stusb4500->sector[4][0] |= ((digitalVoltage & 0x03) << 6);  // load voltage bits 0:1 into bits 6:7
        stusb4500->sector[4][1] = (digitalVoltage >> 2);            // load bits 2:9

        // PDO3
        // Load voltage (10-bit)
        // -bit 8:9 - sector 4, byte 3, bits 0:1
        // -bit 0:7 - sector 4, byte 2, bits 0:7
        digitalVoltage = voltage[2] * 20;                  // convert voltage to 10-bit value
        stusb4500->sector[4][2] = 0xFF & digitalVoltage;   // load bits 0:7
        stusb4500->sector[4][3] &= 0xFC;                   // clear bits 0:1
        stusb4500->sector[4][3] |= (digitalVoltage >> 8);  // load bits 8:9

        // Load highest priority PDO number from memory
        uint8_t buffer;
        stusb4500->i2c_read(stusb4500->addr, DPM_PDO_NUMB, &buffer, 1);

        // load PDO number (stusb4500->sector 3, byte 2, bits 2:3) for NVM saving
        stusb4500->sector[3][2] &= 0xF9;
        stusb4500->sector[3][2] |= (buffer << 1);

        CUST_EnterWriteMode(stusb4500, SECTOR_0 | SECTOR_1 | SECTOR_2 | SECTOR_3 | SECTOR_4);
        CUST_WriteSector(stusb4500, 0, &stusb4500->sector[0][0]);
        CUST_WriteSector(stusb4500, 1, &stusb4500->sector[1][0]);
        CUST_WriteSector(stusb4500, 2, &stusb4500->sector[2][0]);
        CUST_WriteSector(stusb4500, 3, &stusb4500->sector[3][0]);
        CUST_WriteSector(stusb4500, 4, &stusb4500->sector[4][0]);
        CUST_ExitTestMode(stusb4500);
    } else {
        uint8_t default_sector[5][8] = {
            {0x00, 0x00, 0xB0, 0xAA, 0x00, 0x45, 0x00, 0x00},
            {0x10, 0x40, 0x9C, 0x1C, 0xFF, 0x01, 0x3C, 0xDF},
            {0x02, 0x40, 0x0F, 0x00, 0x32, 0x00, 0xFC, 0xF1},
            {0x00, 0x19, 0x56, 0xAF, 0xF5, 0x35, 0x5F, 0x00},
            {0x00, 0x4B, 0x90, 0x21, 0x43, 0x00, 0x40, 0xFB}};

        CUST_EnterWriteMode(stusb4500, SECTOR_0 | SECTOR_1 | SECTOR_2 | SECTOR_3 | SECTOR_4);
        CUST_WriteSector(stusb4500, 0, &default_sector[0][0]);
        CUST_WriteSector(stusb4500, 1, &default_sector[1][0]);
        CUST_WriteSector(stusb4500, 2, &default_sector[2][0]);
        CUST_WriteSector(stusb4500, 3, &default_sector[3][0]);
        CUST_WriteSector(stusb4500, 4, &default_sector[4][0]);
        CUST_ExitTestMode(stusb4500);
    }
}

/**
 * @brief Get the voltage of the specified PDO
 * 
 * @param stusb4500 Pointer to stusb4500_t struct
 * @param pdo_num  PDO number (1-3)
 * @return float Return the voltage of the specified PDO
 */
float stusb4500_get_voltage(stusb4500_t *stusb4500, uint8_t pdo_num) {
    float voltage = 0;
    uint32_t pdoData = stusb4500_read_PDO(stusb4500, pdo_num);

    pdoData = (pdoData >> 10) & 0x3FF;
    voltage = pdoData / 20.0;

    return voltage;
}

/**
 * @brief Get the current of the specified PDO
 * 
 * @param stusb4500 
 * @param pdo_num 
 * @return float Return the current of the specified PDO
 */
float stusb4500_get_current(stusb4500_t *stusb4500, uint8_t pdo_num) {
    uint32_t pdoData = stusb4500_read_PDO(stusb4500, pdo_num);

    pdoData &= 0x3FF;

    return pdoData * 0.01;
}

/**
 * @brief Get the upper voltage limit of the specified PDO
 * 
 * @param stusb4500 
 * @param pdo_num 
 * @return uint8_t Return the upper voltage limit of the specified PDO
 */
uint8_t stusb4500_get_upper_voltage_limit(stusb4500_t *stusb4500, uint8_t pdo_num) {
    if (pdo_num == 1) {
        return (stusb4500->sector[3][3] >> 4) + 5;
    } else if (pdo_num == 2) {
        return (stusb4500->sector[3][5] & 0x0F) + 5;
    } else {
        return (stusb4500->sector[3][6] >> 4) + 5;
    }
}

/**
 * @brief Get the lower voltage limit of the specified PDO
 * 
 * @param stusb4500 
 * @param pdo_num 
 * @return uint8_t Return the lower voltage limit of the specified PDO
 */
uint8_t stusb4500_get_lower_voltage_limit(stusb4500_t *stusb4500, uint8_t pdo_num) {
    if (pdo_num == 1) {
        return 0;
    } else if (pdo_num == 2) {
        return (stusb4500->sector[3][4] >> 4) + 5;
    } else {
        return (stusb4500->sector[3][6] & 0x0F) + 5;
    }
}

/**
 * @brief Get the flex current
 * 
 * @param stusb4500 
 * @return float Return the flex current 
 */
float stusb4500_get_flex_current(stusb4500_t *stusb4500) {
    uint16_t digitalValue = ((stusb4500->sector[4][4] & 0x0F) << 6) + ((stusb4500->sector[4][3] & 0xFC) >> 2);
    return digitalValue / 100.0;
}

/**
 * @brief Get the current PDO number
 * 
 * @param stusb4500 
 * @return uint8_t Return the current PDO number 
 */
uint8_t stusb4500_get_PDO_number(stusb4500_t *stusb4500) {
    uint8_t buffer;
    stusb4500->i2c_read(stusb4500->addr, DPM_PDO_NUMB, &buffer, 1);
    return buffer;
}

/**
 * @brief 
 * 
 * @param stusb4500 
 * @return uint8_t 
 */
uint8_t stusb4500_get_external_power(stusb4500_t *stusb4500) {
    return (stusb4500->sector[3][2] & 0x08) >> 3;
}

uint8_t stusb4500_get_usb_comm_capable(stusb4500_t *stusb4500) {
    return (stusb4500->sector[3][2] & 0x01);
}

uint8_t stusb4500_get_config_ok_gpio(stusb4500_t *stusb4500) {
    return (stusb4500->sector[4][4] & 0x60) >> 5;
}

uint8_t stusb4500_get_gpio_ctrl(stusb4500_t *stusb4500) {
    return (stusb4500->sector[1][0] & 0x30) >> 4;
}

uint8_t stusb4500_get_power_above_5v(stusb4500_t *stusb4500) {
    return (stusb4500->sector[4][6] & 0x08) >> 3;
}

uint8_t stusb4500_get_req_src_current(stusb4500_t *stusb4500) {
    return (stusb4500->sector[4][6] & 0x10) >> 4;
}

void stusb4500_set_voltage(stusb4500_t *stusb4500, uint8_t pdo_num, float voltage) {
    if (pdo_num < 1)
        pdo_num = 1;
    else if (pdo_num > 3)
        pdo_num = 3;

    // Constrain voltage variable to 5-20V
    if (voltage < 5)
        voltage = 5;
    else if (voltage > 20)
        voltage = 20;

    // Load voltage to volatile PDO memory (PDO1 needs to remain at 5V)
    if (pdo_num == 1) voltage = 5;

    voltage *= 20;

    // Replace voltage from bits 10:19 with new voltage
    uint32_t pdoData = stusb4500_read_PDO(stusb4500, pdo_num);

    pdoData &= ~(0xFFC00);
    pdoData |= ((uint32_t) voltage) << 10;

    stusb4500_write_PDO(stusb4500, pdo_num, pdoData);
}

void stusb4500_set_current(stusb4500_t *stusb4500, uint8_t pdo_num, float current) {
    // Load current to volatile PDO memory
    current /= 0.01;

    uint32_t intCurrent = current;
    intCurrent &= 0x3FF;

    uint32_t pdoData = stusb4500_read_PDO(stusb4500, pdo_num);
    pdoData &= ~(0x3FF);
    pdoData |= intCurrent;

    stusb4500_write_PDO(stusb4500, pdo_num, pdoData);
}

void stusb4500_set_upper_voltage_limit(stusb4500_t *stusb4500, uint8_t pdo_num, uint8_t limit) {
    // Constrain value to 5-20%
    if (limit < 5)
        limit = 5;
    else if (limit > 20)
        limit = 20;

    if (pdo_num == 1)  // OVLO1
    {
        // load OVLO (sector 3, byte 3, bits 4:7)
        stusb4500->sector[3][3] &= 0x0F;              // clear bits 4:7
        stusb4500->sector[3][3] |= (limit - 5) << 4;  // load new OVLO value
    } else if (pdo_num == 2)              // OVLO2
    {
        // load OVLO (sector 3, byte 5, bits 0:3)
        stusb4500->sector[3][5] &= 0xF0;         // clear bits 0:3
        stusb4500->sector[3][5] |= (limit - 5);  // load new OVLO value
    } else if (pdo_num == 3)         // OVLO3
    {
        // load OVLO (sector 3, byte 6, bits 4:7)
        stusb4500->sector[3][6] &= 0x0F;
        stusb4500->sector[3][6] |= ((limit - 5) << 4);
    }
}

void stusb4500_set_lower_voltage_limit(stusb4500_t *stusb4500, uint8_t pdo_num, uint8_t limit) {
    // Constrain value to 5-20%
    if (limit < 5)
        limit = 5;
    else if (limit > 20)
        limit = 20;

    // UVLO1 fixed

    if (pdo_num == 2)  // UVLO2
    {
        // load UVLO (sector 3, byte 4, bits 4:7)
        stusb4500->sector[3][4] &= 0x0F;              // clear bits 4:7
        stusb4500->sector[3][4] |= (limit - 5) << 4;  // load new UVLO value
    } else if (pdo_num == 3)              // UVLO3
    {
        // load UVLO (sector 3, byte 6, bits 0:3)
        stusb4500->sector[3][6] &= 0xF0;
        stusb4500->sector[3][6] |= (limit - 5);
    }
}

void stusb4500_set_flex_current(stusb4500_t *stusb4500, float current) {
    // Constrain value to 0-5A
    if (current > 5)
        current = 5;
    else if (current < 0)
        current = 0;

    uint16_t flex_val = current * 100;

    stusb4500->sector[4][3] &= 0x03;                      // clear bits 2:6
    stusb4500->sector[4][3] |= ((flex_val & 0x3F) << 2);  // set bits 2:6

    stusb4500->sector[4][4] &= 0xF0;                       // clear bits 0:3
    stusb4500->sector[4][4] |= ((flex_val & 0x3C0) >> 6);  // set bits 0:3
}

void stusb4500_set_PDO_number(stusb4500_t *stusb4500, uint8_t pdo_num) {
    uint8_t buffer;
    if (pdo_num > 3) pdo_num = 3;

    // load PDO number to volatile memory
    buffer = pdo_num;
    stusb4500->i2c_write(stusb4500->addr, DPM_PDO_NUMB, &buffer, 1);
}

void stusb4500_set_external_power(stusb4500_t *stusb4500, uint8_t external_power) {
    if (external_power != 0) external_power = 1;

    // load SNK_UNCONS_POWER (sector 3, byte 2, bit 3)
    stusb4500->sector[3][2] &= 0xF7;  // clear bit 3
    stusb4500->sector[3][2] |= (external_power) << 3;
}

void stusb4500_set_usb_comm_capable(stusb4500_t *stusb4500, uint8_t usb_comm_capable) {
    if (usb_comm_capable != 0) usb_comm_capable = 1;

    // load USB_COMM_CAPABLE (sector 3, byte 2, bit 0)
    stusb4500->sector[3][2] &= 0xFE;  // clear bit 0
    stusb4500->sector[3][2] |= (usb_comm_capable);
}

void stusb4500_set_config_ok_gpio(stusb4500_t *stusb4500, uint8_t config_ok_gpio) {
    if (config_ok_gpio < 2)
        config_ok_gpio = 0;
    else if (config_ok_gpio > 3)
        config_ok_gpio = 3;

    // load POWER_OK_CFG (sector 4, byte 4, bits 5:6)
    stusb4500->sector[4][4] &= 0x9F;  // clear bit 3
    stusb4500->sector[4][4] |= config_ok_gpio << 5;
}

void stusb4500_set_gpio_ctrl(stusb4500_t *stusb4500, uint8_t gpio_ctrl) {
    if (gpio_ctrl > 3) gpio_ctrl = 3;

    // load GPIO_CFG (sector 1, byte 0, bits 4:5)
    stusb4500->sector[1][0] &= 0xCF;  // clear bits 4:5
    stusb4500->sector[1][0] |= gpio_ctrl << 4;
}

void stusb4500_set_power_above_5v(stusb4500_t *stusb4500, uint8_t power_above_5v) {
    if (power_above_5v != 0) power_above_5v = 1;

    // load POWER_ONLY_ABOVE_5V (sector 4, byte 6, bit 3)
    stusb4500->sector[4][6] &= 0xF7;                   // clear bit 3
    stusb4500->sector[4][6] |= (power_above_5v << 3);  // set bit 3
}

void stusb4500_set_req_src_current(stusb4500_t *stusb4500, uint8_t req_src_current) {
    if (req_src_current != 0) req_src_current = 1;

    // load REQ_SRC_CURRENT (sector 4, byte 6, bit 4)
    stusb4500->sector[4][6] &= 0xEF;                    // clear bit 4
    stusb4500->sector[4][6] |= (req_src_current << 4);  // set bit 4
}

void stusb4500_reset(stusb4500_t *stusb4500) {
    if (stusb4500->hard_reset) {
        stusb4500->hard_reset_high();
        stusb4500->delay_ms(10);
        stusb4500->hard_reset_low();
    } else {
        uint8_t buffer;

        // Soft Reset
        buffer = 0x0D;  // SOFT_RESET
        stusb4500->i2c_write(stusb4500->addr, TX_HEADER_LOW, &buffer, 1);

        buffer = 0x26;  // SEND_COMMAND
        stusb4500->i2c_write(stusb4500->addr, PD_COMMAND_CTRL, &buffer, 1);
    }
}

uint32_t stusb4500_read_PDO(stusb4500_t *stusb4500, uint8_t pdo_num) {
    uint32_t pdoData = 0;
    uint8_t buffer[4];

    // PDO1:0x85, PDO2:0x89, PDO3:0x8D
    stusb4500->i2c_read(stusb4500->addr, 0x85 + ((pdo_num - 1) * 4), buffer, 4);

    // Combine the 4 buffer bytes into one 32-bit integer
    for (uint8_t i = 0; i < 4; i++) {
        uint32_t tempData = buffer[i];
        tempData = (tempData << (i * 8));
        pdoData += tempData;
    }

    return pdoData;
}

void stusb4500_write_PDO(stusb4500_t *stusb4500, uint8_t pdo_num, uint32_t pdoData) {
    uint8_t buffer[4];

    buffer[0] = (pdoData)&0xFF;
    buffer[1] = (pdoData >> 8) & 0xFF;
    buffer[2] = (pdoData >> 16) & 0xFF;
    buffer[3] = (pdoData >> 24) & 0xFF;

    stusb4500->i2c_write(stusb4500->addr, 0x85 + ((pdo_num - 1) * 4), buffer, 4);
}

uint8_t CUST_EnterWriteMode(stusb4500_t *stusb4500, unsigned char ErasedSector) {
    uint8_t buffer;

    buffer = FTP_CUST_PASSWORD; /* Set Password*/
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CUST_PASSWORD_REG, &buffer, 1) != 0) return -1;

    buffer = 0; /* this register must be NULL for Partial Erase feature */
    if (stusb4500->i2c_write(stusb4500->addr, RW_BUFFER, &buffer, 1) != 0) return -1;

    {
        // NVM Power-up Sequence
        // After STUSB start-up sequence, the NVM is powered off.

        buffer = 0; /* NVM internal controller reset */
        if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1;

        buffer = FTP_CUST_PWR | FTP_CUST_RST_N; /* Set PWR and RST_N bits */
        if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1;
    }

    buffer = ((ErasedSector << 3) & FTP_CUST_SER) | (WRITE_SER & FTP_CUST_OPCODE);     /* Load 0xF1 to erase all sectors of FTP and Write SER Opcode */
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_1, &buffer, 1) != 0) return -1; /* Set Write SER Opcode */

    buffer = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Load Write SER Opcode */

    do {
        stusb4500->delay_ms(500);
        if (stusb4500->i2c_read(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Wait for execution */
    } while (buffer & FTP_CUST_REQ);

    buffer = SOFT_PROG_SECTOR & FTP_CUST_OPCODE;
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_1, &buffer, 1) != 0) return -1; /* Set Soft Prog Opcode */

    buffer = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Load Soft Prog Opcode */

    do {
        if (stusb4500->i2c_read(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Wait for execution */
    } while (buffer & FTP_CUST_REQ);

    buffer = ERASE_SECTOR & FTP_CUST_OPCODE;
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_1, &buffer, 1) != 0) return -1; /* Set Erase Sectors Opcode */

    buffer = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Load Erase Sectors Opcode */

    do {
        if (stusb4500->i2c_read(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Wait for execution */
    } while (buffer & FTP_CUST_REQ);

    return 0;
}

uint8_t CUST_ExitTestMode(stusb4500_t *stusb4500) {
    uint8_t buffer[2];

    buffer[0] = FTP_CUST_RST_N;
    buffer[1] = 0x00; /* clear registers */
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, buffer, 1) != 0) return -1;

    buffer[0] = 0x00;
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CUST_PASSWORD_REG, buffer, 1) != 0) return -1; /* Clear Password */

    return 0;
}

uint8_t CUST_WriteSector(stusb4500_t *stusb4500, char SectorNum, unsigned char *SectorData) {
    uint8_t buffer;

    // Write the 64-bit data to be written in the sector
    if (stusb4500->i2c_write(stusb4500->addr, RW_BUFFER, SectorData, 8) != 0) return -1;

    buffer = FTP_CUST_PWR | FTP_CUST_RST_N; /*Set PWR and RST_N bits*/
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1;

    // NVM Program Load Register to write with the 64-bit data to be written in sector
    buffer = (WRITE_PL & FTP_CUST_OPCODE); /*Set Write to PL Opcode*/
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_1, &buffer, 1) != 0) return -1;

    buffer = FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ; /* Load Write to PL Sectors Opcode */
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1;

    do {
        if (stusb4500->i2c_read(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Wait for execution */
    } while (buffer & FTP_CUST_REQ);                                                      // FTP_CUST_REQ clear by NVM controller

    // NVM "Word Program" operation to write the Program Load Register in the sector to be written
    buffer = (PROG_SECTOR & FTP_CUST_OPCODE);
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_1, &buffer, 1) != 0) return -1; /*Set Prog Sectors Opcode*/

    buffer = (SectorNum & FTP_CUST_SECT) | FTP_CUST_PWR | FTP_CUST_RST_N | FTP_CUST_REQ;
    if (stusb4500->i2c_write(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Load Prog Sectors Opcode */

    do {
        if (stusb4500->i2c_read(stusb4500->addr, FTP_CTRL_0, &buffer, 1) != 0) return -1; /* Wait for execution */
    } while (buffer & FTP_CUST_REQ);                                                      // FTP_CUST_REQ clear by NVM controller

    return 0;
}