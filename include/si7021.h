#pragma once

#include "hardware/i2c.h"

#define SI7021_ADDR 0x40

// define the enum for the si7021 error states
typedef enum
{
    SI7021_OK,
    SI7021_ERR_NO_I2C,
    SI7021_ERR_NO_RESPONSE,
    SI7021_ERR_INVALID_HEATER_LEVEL,
    SI7021_ERR_WRITE_FAIL,
    SI7021_ERR_READ_FAIL,
    SI7021_ERR_CRC_FAIL,
} si7021_error_t;

typedef struct
{
    uint8_t serial_buffer[8];
} si7021_serial_num_t;

typedef struct si7021
{
    i2c_inst_t *i2c;
    bool hold_master;
} si7021_t;

typedef struct
{
    float temperature;
    float humidity;
} si7021_reading_t;

si7021_error_t si7021_init(si7021_t *sensor);

si7021_error_t read_temperature(si7021_t *sensor, si7021_reading_t *reading);

si7021_error_t read_humidity(si7021_t *sensor, si7021_reading_t *reading);

si7021_error_t read_temperature_humidity(si7021_t *sensor, si7021_reading_t *reading);

si7021_error_t si7021_reset(si7021_t *sensor);

si7021_error_t si7021_set_defaults(si7021_t *sensor);

si7021_error_t si7021_enable_heater(si7021_t *sensor);

si7021_error_t si7021_disable_heater(si7021_t *sensor);

si7021_error_t si7021_is_heater_enabled(si7021_t *sensor, bool *heater_is_enabled);

si7021_error_t si7021_set_heater_level(si7021_t *sensor, uint8_t heater_level);

si7021_error_t si7021_get_heater_level(si7021_t *sensor, uint8_t *heater_level);

si7021_error_t si7021_get_serial_num(si7021_t *sensor, si7021_serial_num_t *serial_num);

si7021_error_t si7021_print_serial_num(si7021_serial_num_t *serial_num, char *buffer);

si7021_error_t si7021_get_firmware_rev(si7021_t *sensor, uint8_t *firmware);
