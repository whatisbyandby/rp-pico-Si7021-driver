#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/i2c.h"
#include "si7021.h"

int main() {

    stdio_init_all();

    FILE *fp;

    // Initalize the I2C bus
    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);

    // Initialize the SI7021 sensor
    si7021_t sensor = {
        .i2c = i2c0,
    };
    
    si7021_error_t err = si7021_init(&sensor);


    while (true) {
        si7021_reading_t reading;
        si7021_error_t err = read_temperature(&sensor, &reading);
        err = read_humidity(&sensor, &reading);

        if (err != SI7021_OK) {
            printf("Error reading temperature: %d\n", err);
        } else {
            printf("Temperature: %.2f\n", reading.temperature);
            printf("Humidity: %.2f\n", reading.humidity);
        }

        sleep_ms(1000);
    }
}