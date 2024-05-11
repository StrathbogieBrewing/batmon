#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvm.h"
#include "storage.h"

#define STORAGE_MAGIC (0xBEEF)

#define STORAGE_TASK_PRIO 8

#define STORAGE_BLOCK_SIZE (4096)

static const char *TAG = "storage";

typedef struct storage_header_t {
    uint16_t magic;
    uint16_t crc;
} storage_header_t;

#define STORAGE_DATA_SIZE (STORAGE_BLOCK_SIZE - sizeof(storage_header_t))

typedef struct storage_block_t {
    storage_header_t header;
    uint8_t data[STORAGE_DATA_SIZE];
} storage_block_t;

typedef struct storage_ctx_t {
    SemaphoreHandle_t mutex;
    nvm_device_t *device;

    storage_block_t cache_buffer;
    storage_block_t read_buffer;
    storage_block_t write_buffer;

    uint16_t block_count;
    uint16_t block_size;

    uint16_t buffer_size;
    uint16_t write_buffer_index;
    uint16_t read_buffer_index;

    uint32_t read_block;
    uint32_t write_block;

} storage_ctx_t;

static storage_ctx_t storage_ctx = {0};

static uint16_t storage_crc16(uint8_t *buffer, uint16_t crc, uint16_t size) {
    for (uint16_t index = 0; index < size; index++) {
        crc = crc ^ buffer[index];
        for (uint8_t i = 0; i < 8; ++i) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc = (crc >> 1);
        }
    }
    return crc;
}

static nvm_err_t storage_crc_block(storage_block_t *block) {
    return storage_crc16(block->data, 0xFFFF, STORAGE_DATA_SIZE);
}

static nvm_err_t storage_check_block(storage_block_t *block) {
    nvm_err_t error = NVM_OK;
    if (block->header.magic != STORAGE_MAGIC) {
        error = NVM_FAIL;
    }
    if (block->header.crc != storage_crc_block(block)) {
        error = NVM_FAIL;
    }
    return error;
}

static void storage_write(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE) {
        handle->write_buffer.header.magic = STORAGE_MAGIC;
        handle->write_buffer.header.crc = storage_crc_block(block);
        error = handle->device->write(handle->write_block, (uint8_t *)&handle->write_buffer);
        xSemaphoreGive(handle->mutex);
    } else {
        error = NVM_FAIL;
    }
    return error;
}

nvm_err_t storage_format(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE) {
        error = handle->device->erase(0, handle->device->sector_count);
        xSemaphoreGive(handle->mutex);
    } else {
        error = NVM_FAIL;
    }
    return error;
}

nvm_err_t storage_read(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE) {
        error = handle->device->read(handle->read_block, (uint8_t *)&handle->read_buffer);
        if (error == NVM_OK) {
            error = storage_check_block(&handle->read_buffer);
        }
        xSemaphoreGive(handle->mutex);
    } else {
        error = NVM_FAIL;
    }
    return error;
}

nvm_err_t storage_init(storage_handle_t *handle, nvm_device_t *device) {
    nvm_err_t error = NVM_OK;
    *handle = &storage_ctx;
    (*handle)->device = device;
    (*handle)->device->init();
    (*handle)->mutex = xSemaphoreCreateMutex();
    (*handle)->block_count = (*handle)->device->sector_count;
    (*handle)->block_size = (*handle)->device->sector_size;

    (*handle)->read_block = 0; // check if media is formatted
    if (storage_read(*handle) != NVM_OK) {
        storage_format(*handle);
    }

    uint32_t low_block = 1; // binary search for the first unused block
    uint32_t high_block = (*handle)->block_count - 1;
    while (low_block != high_block) {
        uint32_t mid_block = low_block + (high_block - low_block) / 2;
        (*handle)->read_block = mid_block;
        if (storage_read(*handle) == NVM_OK) {
            low_block = mid_block + 1;
        } else {
            high_block = mid_block;
        }
    }
    (*handle)->write_block = low_block; // initialise read and write blocks
    (*handle)->write_buffer_index = 0;
    (*handle)->read_block = low_block - 1;
    (*handle)->read_buffer_index = 0;
    return error;
}

nvm_err_t storage_read_sync(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    if (xSemaphoreTake(handle->mutex, portMAX_DELAY) == pdTRUE) {
        handle->read_block = handle->write_block - 1;
        memcpy(&handle->read_buffer, &handle->write_buffer, handle->block_size);
        handle->read_buffer_index = handle->write_buffer_index;
        if (handle->read_buffer_index == 0) {
            error = storage_read(*handle);
        }
        xSemaphoreGive(handle->mutex);
    } else {
        error = NVM_FAIL;
    }
    return error;
}

nvm_err_t storage_read_str(storage_handle_t handle, char *s, size_t maxlen) {
    nvm_err_t err = nvm_read(uint32_t block_index, nvm_block_t * block);
    return NVM_OK;
}

nvm_err_t storage_write_str(storage_handle_t handle, const char *s, size_t maxlen) {
    nvm_err_t err = nvm_write(uint32_t block_index, nvm_block_t * block);
    return NVM_OK;
}

nvm_err_t storage_write_sync(storage_handle_t handle) { return NVM_OK; }

// }
// if (error == NVM_OK) {
//     if ((STORAGE_BLOCK_SIZE % handle->device->block_size != 0) ||
//         (STORAGE_BLOCK_SIZE < handle->device->block_size)) {
//         ESP_LOGE(TAG, "init failed block size");
//         error = NVM_FAIL;
//     } else {
// (*handle)->sectors_per_block = STORAGE_BLOCK_SIZE / (*handle)->device->block_size;
// }
// }
// if (error == NVM_OK) {

// check if the first block has been initialed
// nvm_err_t err = (*handle)->device->read(0, (nvm_block_t)&handle->cache_buffer);
// if (err != NVM_OK) {
//     ESP_LOGE(TAG, "init failed nvm_read");
//     error = NVM_FAIL;
// }
// if (storage_check_block(&handle->cache_buffer) != NVM_OK) {
//     nvm_erase(0, handle->nvm_info.block_count); // erase everything
//     memset(&handle->cache_buffer, '\0', NVM_BLOCK_SIZE);
//     storage_write_block(0, &handle->cache_buffer);
//     handle->write_block = 1;
//     handle->ready = true;
//     ESP_LOGI(TAG, "init erased everything");
//     ESP_LOGI(TAG, "init first empty block  %" PRIx32, handle->write_block);
// } else {
//     uint32_t low_block = 1; // binary search for first unused block
//     uint32_t high_block = handle->nvm_info.block_count;
//     while (low_block != high_block) {
//         uint32_t mid_block = low_block + (high_block - low_block) / 2;
//         nvm_read(mid_block, (nvm_block_t)&handle->cache_buffer);
//         if (handle->cache_buffer.header.magic == STORAGE_MAGIC) {
//             low_block = mid_block + 1;
//         } else {
//             high_block = mid_block;
//         }
//     }
//     // if(low_block == handle->nvm_info.block_count){
//     //     ESP_LOGE(TAG, "init failed full");
//     //     error = STORAGE_FULL;
//     // } else {
//     handle->write_block = low_block;
//     handle->read_block = low_block - 1;
//     handle->ready = true;
//     ESP_LOGI(TAG, "init first empty block %" PRIx32, handle->write_block);
//     // }
// }
// }

// if(low_block == handle->nvm_info.block_count){
//     ESP_LOGE(TAG, "init failed full");
//     return STORAGE_FULL;
// }
// handle->write_block = low_block;
// handle->read_block = low_block - 1;
// handle->ready = true;
// ESP_LOGI(TAG, "init first empty block %" PRIx32, handle->write_block);
// *handle = &storage_handle;
// return error;