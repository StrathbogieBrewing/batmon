/* Finding Partitions Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_log.h"
#include "esp_partition.h"
#include <assert.h>
#include <string.h>

#include "storage.h"
#include "nvm_esp.h"

static const char *TAG = "example";

void app_main(void) {
    storage_handle_t storage;

    ESP_LOGI(TAG, "Find partition by specifying the label");
    storage_open(&storage, &nvm_esp);



    
}
