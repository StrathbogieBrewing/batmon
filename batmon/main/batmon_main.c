#include "esp_check.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
// #include "nvs_flash.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "batmon_littlefs.h"
#include "batmon_wifi.h"
#include "driver/gpio.h"
#include "hardware.h"
#include "rest_server.h"
#include "sntp_client.h"
#include "tinbus.h"

#include <sys/time.h>
#include <time.h>

static const char *TAG = "batmon";

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(batmon_wifi_start_station());

    hardware_init();
    // batmon_littlefs_init();
    batmon_littlefs_mount_sdspi();
    ESP_ERROR_CHECK(start_rest_server(BATMON_LITTLEFS_BASE_PATH));

    sntp_client_init();

    ESP_ERROR_CHECK(tinbus_init());

    ESP_LOGI(TAG, "Tinbus installed on GPIO%d", HARDWARE_BUS_RX_GPIO);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));

        tinbus_msg_t msg;
        if (tinbus_read(&msg) == ESP_OK) {
            hardware_debug(HARDWARE_GREENLED);
            if (sntp_time_is_set()) {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                char strftime_buf[64];
                struct tm timeinfo;
                localtime_r(&tv.tv_sec, &timeinfo);
                strftime(strftime_buf, sizeof(strftime_buf), "%y%m%d-%H%M%S", &timeinfo);
                ESP_LOGI(TAG, "%s.%3.3ld Device %2.2d = %d", strftime_buf, tv.tv_usec / 1000, msg.device_id, msg.value);
                ESP_LOGI(TAG, "Free heap %d", xPortGetFreeHeapSize());
            }
        }
    }
}
