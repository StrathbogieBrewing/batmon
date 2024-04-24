
// #include "esp_log.h"
#include "driver/gpio.h"

#include "hardware.h"

#define GPIO_OUTPUT_PIN_SEL                                                                                            \
    ((1ULL << HARDWARE_RELAY1) | (1ULL << HARDWARE_RELAY2) | (1ULL << HARDWARE_RELAY3) | (1ULL << HARDWARE_AMBERLED) |  \
        (1ULL << HARDWARE_GREENLED) | (1ULL << HARDWARE_HEADERPIN1) | (1ULL << HARDWARE_HEADERPIN3) |                  \
        (1ULL << HARDWARE_HEADERPIN7) | (1ULL << HARDWARE_HEADERPIN9))

esp_err_t hardware_init(void) {
    // Configure the output pins
    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;      // disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;            // set as output mode
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; // bit mask of the pins to set
    io_conf.pull_down_en = 0;                   // disable pull-down mode
    io_conf.pull_up_en = 0;                     // disable pull-up mode
    esp_err_t err = gpio_config(&io_conf);
    return err;
}

void hardware_debug(gpio_num_t  gpio_num) {
    static bool state[64] = {0};
    if (state[gpio_num]) {
        state[gpio_num] = false;
        gpio_set_level(gpio_num, false);
    } else {
        state[gpio_num] = true;
        gpio_set_level(gpio_num, true);
    }
}
