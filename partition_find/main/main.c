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

// static nvm_block_t block;

void app_main(void) {
    storage_handle_t storage;

    ESP_LOGI(TAG, "Find partition by specifying the label");
    storage_init(&storage, &nvm_esp);

    // // block.index = 0;
    // nvm_read(&block);
    // ESP_LOG_BUFFER_HEX_LEVEL(TAG, block.data, 16, ESP_LOG_INFO);

    // nvm_erase(0, 1);

    // nvm_read(&block);
    // ESP_LOG_BUFFER_HEX_LEVEL(TAG, block.data, 16, ESP_LOG_INFO);

    // for (uint8_t i = 0; i < 10; i++) {
    //     block.data[i] = i;
    // }

    // ESP_LOG_BUFFER_HEX_LEVEL(TAG, block.data, 16, ESP_LOG_INFO);
    // nvm_write(&block);

    // nvm_read(&block);
    // ESP_LOG_BUFFER_HEX_LEVEL(TAG, block.data, 16, ESP_LOG_INFO);

    
}
