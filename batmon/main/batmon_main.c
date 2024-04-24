#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "hardware.h"
#include "tinbus.h"

static const char *TAG = "batmon";

void app_main(void) {
    hardware_init();

    // gpio_set_level(HARDWARE_GREENLED, 1);

    tinbus_init(); // never returns

    // ESP_ERROR_CHECK(tinbus_init());
    // ESP_LOGI(TAG, "Tinbus installed on GPIO%d", HARDWARE_BUS_RX_GPIO);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));

        tinbus_msg_t msg;
        if (tinbus_read(&msg) == ESP_OK) {
            hardware_debug(HARDWARE_GREENLED);
            ESP_LOGI(TAG, "Device %2.2d = %d", msg.device_id, msg.value);
        }
    }
}
