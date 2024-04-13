#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "si7021.h"
#include "string.h"
#include <stdio.h>

// Commands for the Si7021
#define SI7021_MEASURE_TEMP_HOLD 0xE3
#define SI7021_MEASURE_HUM_HOLD 0xE5
#define SI7021_MEASURE_TEMP_NO_HOLD 0xF3
#define SI7021_MEASURE_HUM_NO_HOLD 0xF5
#define SI7021_READ_TEMP_PREV 0xE0
#define SI7021_RESET 0xFE
#define SI7021_READ_USER_REG 0xE7
#define SI7021_WRITE_USER_REG 0xE6
#define SI7021_READ_HEATER_REG 0x11
#define SI7021_WRITE_HEATER_REG 0x51

// Default register values
#define SI7021_USER_REG_DEFAULT 0x3A
#define SI7021_HEATER_REG_DEFAULT 0x00

bool verify_crc(uint8_t *data, uint8_t received_crc, uint8_t len)
{
    uint8_t crc = 0x00;     // CRC initialization value
    int poly = 0b100110001; // Polynomial: x^8 + x^5 + x^4 + 1 (0x31)
    uint8_t i, j;

    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (j = 8; j > 0; j--)
        {
            if (crc & 0x80)              // if the highest bit is 1...
                crc = (crc << 1) ^ poly; // shift left and XOR with the polynomial
            else
                crc = (crc << 1); // just shift left
        }
    }
    return crc = received_crc; // Compare the calculated CRC with the received CRC
}

si7021_error_t write_then_read(si7021_t *sensor, uint8_t command, uint16_t *result)
{
    uint8_t read_buffer[2] = {0, 0};
    int num_bytes = i2c_write_blocking(sensor->i2c, SI7021_ADDR, &command, 1, false);

    if (num_bytes != 1)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    num_bytes = i2c_read_blocking(sensor->i2c, SI7021_ADDR, read_buffer, 3, false);

    if (num_bytes != 3)
    {
        return SI7021_ERR_READ_FAIL;
    }

    if (!verify_crc(read_buffer, read_buffer[2], 2))
    {
        return SI7021_ERR_CRC_FAIL;
    }

    *result = (read_buffer[0] << 8) | read_buffer[1];
    return SI7021_OK;
}

si7021_error_t read_temperature(si7021_t *sensor, si7021_reading_t *reading)
{

    uint16_t temp_code;
    uint8_t command = sensor->hold_master ? SI7021_MEASURE_TEMP_HOLD : SI7021_MEASURE_TEMP_NO_HOLD;
    si7021_error_t err = write_then_read(sensor, SI7021_MEASURE_TEMP_HOLD, &temp_code);

    if (err != SI7021_OK)
    {
        return err;
    }

    reading->temperature = (175.72 * temp_code / 65536) - 46.85;
    return SI7021_OK;
}

si7021_error_t read_humidity(si7021_t *sensor, si7021_reading_t *reading)
{

    uint16_t hum_code;
    uint8_t command = sensor->hold_master ? SI7021_MEASURE_HUM_HOLD : SI7021_MEASURE_HUM_NO_HOLD;
    si7021_error_t err = write_then_read(sensor, command, &hum_code);

    if (err != SI7021_OK)
    {
        return err;
    }

    // convert the humidity code to a percentage and store in the new reading
    reading->humidity = (125 * hum_code / 65536) - 6;
    return SI7021_OK;
}

si7021_error_t read_temperature_humidity(si7021_t *sensor, si7021_reading_t *reading)
{
    si7021_error_t err = read_humidity(sensor, reading);
    if (err != SI7021_OK)
    {
        return err;
    }

    uint16_t temp_code;
    err = write_then_read(sensor, SI7021_MEASURE_TEMP_HOLD, &temp_code);

    if (err != SI7021_OK)
    {
        return err;
    }

    reading->temperature = (175.72 * temp_code / 65536) - 46.85;
    return SI7021_OK;
}

si7021_error_t si7021_reset(si7021_t *sensor)
{
    uint8_t command[1] = {SI7021_RESET};
    int num_bytes = i2c_write_blocking(sensor->i2c, SI7021_ADDR, command, 1, false);

    if (num_bytes != 1)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    return SI7021_OK;
}

si7021_error_t si7021_set_defaults(si7021_t *sensor)
{

    // reset user register 1
    uint8_t write_buffer[2] = {SI7021_WRITE_USER_REG, SI7021_USER_REG_DEFAULT};
    int num_bytes = i2c_write_blocking(sensor->i2c, SI7021_ADDR, write_buffer, 2, true);

    if (num_bytes != 2)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    // reset heater register
    uint8_t write_buffer_1[2] = {SI7021_WRITE_HEATER_REG, SI7021_HEATER_REG_DEFAULT};
    int num_bytes_1 = i2c_write_blocking(sensor->i2c, SI7021_ADDR, write_buffer_1, 2, true);

    if (num_bytes_1 != 2)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    return SI7021_OK;
}

si7021_error_t read_user_register(si7021_t *sensor, uint8_t *user_reg)
{
    uint8_t write_buffer[1] = {SI7021_READ_USER_REG};
    int num_bytes = i2c_write_blocking(sensor->i2c, SI7021_ADDR, write_buffer, 1, true);

    if (num_bytes != 1)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    uint8_t read_buffer[1] = {0};
    num_bytes = i2c_read_blocking(sensor->i2c, SI7021_ADDR, read_buffer, 1, false);

    if (num_bytes != 1)
    {
        return SI7021_ERR_READ_FAIL;
    }

    *user_reg = read_buffer[0];
    return SI7021_OK;
}

si7021_error_t write_user_register(si7021_t *sensor, uint8_t user_reg)
{
    uint8_t write_buffer[2] = {SI7021_WRITE_USER_REG, user_reg};
    int num_bytes = i2c_write_blocking(sensor->i2c, SI7021_ADDR, write_buffer, 2, true);

    if (num_bytes != 2)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    return SI7021_OK;
}

si7021_error_t si7021_is_heater_enabled(si7021_t *sensor, bool *heater_is_enabled)
{

    uint8_t user_register;
    si7021_error_t err = read_user_register(sensor, &user_register);

    *heater_is_enabled = (user_register & (1 << 2)) != 0;
    return SI7021_OK;
}

si7021_error_t si7021_enable_heater(si7021_t *sensor)
{

    uint8_t user_reg;
    si7021_error_t err = read_user_register(sensor, &user_reg);

    uint8_t updated_reg = user_reg | (1 << 2);

    err = write_user_register(sensor, updated_reg);

    if (err != SI7021_OK)
    {
        return err;
    }

    return SI7021_OK;
}

si7021_error_t si7021_disable_heater(si7021_t *sensor)
{

    uint8_t user_reg;
    si7021_error_t err = read_user_register(sensor, &user_reg);

    user_reg = user_reg & ~(1 << 2);

    err = write_user_register(sensor, user_reg);

    if (err != SI7021_OK)
    {
        return err;
    }

    return SI7021_OK;
}

si7021_error_t si7021_set_heater_level(si7021_t *sensor, uint8_t heater_level)
{
    if (heater_level > 15)
    {
        return SI7021_ERR_INVALID_HEATER_LEVEL;
    }

    uint8_t write_buffer[2] = {SI7021_WRITE_HEATER_REG, heater_level};
    int num_bytes = i2c_write_blocking(sensor->i2c, SI7021_ADDR, write_buffer, 2, true);

    if (num_bytes != 2)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    return SI7021_OK;
}

si7021_error_t si7021_get_heater_level(si7021_t *sensor, uint8_t *heater_level)
{
    uint8_t write_buffer[1] = {SI7021_READ_HEATER_REG};
    int num_bytes = i2c_write_blocking(sensor->i2c, SI7021_ADDR, write_buffer, 1, false);

    if (num_bytes != 1)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    uint8_t read_buffer[1] = {0};
    num_bytes = i2c_read_blocking(sensor->i2c, SI7021_ADDR, read_buffer, 1, false);

    if (num_bytes != 1)
    {
        return SI7021_ERR_READ_FAIL;
    }

    *heater_level = read_buffer[0];
    return SI7021_OK;
}

si7021_error_t si7021_get_serial_num(si7021_t *sensor, si7021_serial_num_t *serial_num)
{

    const uint8_t first_command[2] = {0xFA, 0x0F};
    const uint8_t second_command[2] = {0xFC, 0xC9};

    int first_bytes_written = i2c_write_blocking(sensor->i2c, SI7021_ADDR, first_command, 2, false);

    if (first_bytes_written != 2)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    uint8_t sna[8] = {0};
    int sna_bytes_read = i2c_read_blocking(sensor->i2c, SI7021_ADDR, sna, 8, false);

    if (sna_bytes_read != 8)
    {
        return SI7021_ERR_READ_FAIL;
    }

    serial_num->serial_buffer[0] = sna[0];
    serial_num->serial_buffer[1] = sna[2];
    serial_num->serial_buffer[2] = sna[4];
    serial_num->serial_buffer[3] = sna[6];

    bool valid_read = verify_crc(serial_num->serial_buffer, sna[7], 4);

    if (!valid_read)
    {
        return SI7021_ERR_CRC_FAIL;
    }

    int second_bytes_written = i2c_write_blocking(sensor->i2c, SI7021_ADDR, second_command, 2, false);

    if (second_bytes_written != 2)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    uint8_t snb[6] = {0};
    int snb_bytes_read = i2c_read_blocking(sensor->i2c, SI7021_ADDR, snb, 6, false);

    if (snb_bytes_read != 6)
    {
        return SI7021_ERR_READ_FAIL;
    }

    serial_num->serial_buffer[4] = snb[0];
    serial_num->serial_buffer[5] = snb[1];
    serial_num->serial_buffer[6] = snb[3];
    serial_num->serial_buffer[7] = snb[4];

    valid_read = verify_crc(serial_num->serial_buffer + 4, snb[5], 4);

    if (!valid_read)
    {
        return SI7021_ERR_CRC_FAIL;
    }

    return SI7021_OK;
}

si7021_error_t si7021_print_serial_num(si7021_serial_num_t *serial_num, char *buffer)
{
    if (buffer == NULL)
    {
        return SI7021_ERR_READ_FAIL;
    }

    for (int i = 0; i < 8; i++)
    {
        sprintf(buffer + (i * 2), "%02X", serial_num->serial_buffer[i]);
    }

    return SI7021_OK;
}

si7021_error_t si7021_get_firmware_rev(si7021_t *sensor, uint8_t *firmware)
{
    uint8_t write_buffer[2] = {0x84, 0xB8};
    int num_bytes_written = i2c_write_blocking(sensor->i2c, SI7021_ADDR, write_buffer, 2, true);

    if (num_bytes_written != 2)
    {
        return SI7021_ERR_WRITE_FAIL;
    }

    uint8_t read_buffer[1] = {0};
    int num_bytes_read = i2c_read_blocking(sensor->i2c, SI7021_ADDR, read_buffer, 1, false);

    if (num_bytes_read != 1)
    {
        return SI7021_ERR_READ_FAIL;
    }

    *firmware = read_buffer[0];
    return SI7021_OK;
}

si7021_error_t si7021_init(si7021_t *sensor)
{
    if (sensor->i2c == NULL)
    {
        return SI7021_ERR_NO_I2C;
    }

    uint8_t read_buffer[1] = {0};
    int num_bytes = i2c_read_blocking(i2c0, SI7021_ADDR, read_buffer, 1, false);

    if (num_bytes != 1)
    {
        return SI7021_ERR_NO_RESPONSE;
    }

    return SI7021_OK;
}
