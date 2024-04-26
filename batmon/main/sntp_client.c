#include "esp_attr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "lwip/ip_addr.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define CONFIG_SNTP_TIME_SERVER "time.google.com"
#define SNTP_RETRIES_MAX 10

static const char *TAG = "sntp";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

static bool sntp_time_set = false;

void time_sync_notification_cb(struct timeval *tv) { ESP_LOGI(TAG, "Notification of a time synchronization event"); }

bool sntp_time_is_set(void) { return sntp_time_set; }

void sntp_client_init(void) {

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
        config.sync_cb = time_sync_notification_cb; // Note: This is only needed if we want
        esp_netif_sntp_init(&config);
        uint8_t retry = 0;
        while (retry++ < SNTP_RETRIES_MAX) {
            if(esp_netif_sntp_sync_wait(5000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT){
                ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, SNTP_RETRIES_MAX);
            } else  {
                sntp_time_set = true;
                break;
            }
        }
    }

    char strftime_buf[64];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
}
