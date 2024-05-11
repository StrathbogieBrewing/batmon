#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdcard.h"

typedef struct sdcard_ctx_t {
    esp_partition_t *partition;

} sdcard_ctx_t;

sdcard_ctx_t ctx = {0};

sdcard_err_t sdcard_init(void) {
    ctx.partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "custom");
    if (!partition) {
        ESP_LOGE(TAG, "partition could not be found");
        return ESP_ERR_NOT_FOUND;
    }
}
sdcard_err_t sdcard_read(uint32_t block, sdcard_data_t *data) {
    esp_err_t err = esp_partition_read(partition, block * SDCARD_BLOCK_SIZE, data, SDCARD_BLOCK_SIZE);
}
sdcard_err_t sdcard_write(uint32_t block, sdcard_data_t *data);
sdcard_err_t sdcard_erase(uint32_t block);

#define SDCARD_QUEUE_LENGTH 4
static QueueHandle_t sdcard_queue = 0;

typedef enum {
    SDCARD_READ = 0,
    SDCARD_WRITE,
    SDCARD_ERASE,
    SDCARD_RESET,
} sdcard_action_t;

typedef struct sdcard_request_t {
    sdcard_action_t action;
    uint8_t *buf;
    uint16_t size;
    uint32_t block;
    uint16_t offset;
    callback;
} sdcard_request_t;

static void sdcard_task(void *arg) { sdcard_queue = xQueueCreate(SDCARD_QUEUE_LENGTH, sizeof(sdcard_request_t)); }

sdcard_err_t sdcard_init(void) {

    xTaskCreatePinnedToCore(sdcard_task, "sdcard", 4096, NULL, SDCARD_TASK_PRIO, NULL, 1);

    // find empty block

    // initialise write buffer
    memset(ctx.wr_buffer, 0x00, SDCARD_BLOCK_SIZE);
    ctx.wr_buffer_size = 0;
    return SDCARD_OK;
}