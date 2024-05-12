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

#define STORAGE_TIMEOUT (1000 / portTICK_PERIOD_MS)

static const char *TAG = "storage";

typedef struct storage_header_t {
    uint16_t magic;
    uint16_t crc;
} storage_header_t;

#define STORAGE_BLOCK_SIZE (4096)
#define STORAGE_DATA_SIZE (STORAGE_BLOCK_SIZE - sizeof(storage_header_t))

typedef struct storage_block_t {
    storage_header_t header;
    uint8_t data[STORAGE_DATA_SIZE];
} storage_block_t;

typedef struct storage_buffer_t {
    storage_block_t block;
    uint16_t index;
} storage_buffer_t;

typedef struct storage_ctx_t {
    SemaphoreHandle_t mutex;
    nvm_device_t *device;

    storage_buffer_t read_buffer;
    storage_buffer_t write_buffer;

    uint32_t read_block_index;
    uint32_t write_block_index;

} storage_ctx_t;

static storage_ctx_t storage_ctx = {0};

static uint16_t storage_crc16(uint8_t buffer[], uint16_t size) {
    uint16_t crc = 0xFFFF;
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

static nvm_err_t storage_write_block(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    handle->write_buffer.block.header.magic = STORAGE_MAGIC;
    handle->write_buffer.block.header.crc =
        storage_crc16(handle->write_buffer.block.data, STORAGE_DATA_SIZE);
    error =
        handle->device->write(handle->write_block_index, (uint8_t *)&handle->write_buffer.block);

    memset(&handle->write_buffer, 0, sizeof(handle->write_buffer));
    handle->write_block_index += 1;
    return error;
}

static nvm_err_t storage_read_block(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    error = handle->device->read(handle->read_block_index, (uint8_t *)&handle->read_buffer.block);
    if (error == NVM_OK) {
        // test if block is valid, invalid or erased
        if (handle->read_buffer.block.header.magic != STORAGE_MAGIC) {
            error = NVM_FAIL; // wrong magic number
        }
        if (handle->read_buffer.block.header.crc !=
            storage_crc16(handle->read_buffer.block.data, STORAGE_DATA_SIZE)) {
            error = NVM_FAIL; // bad crc
        }

        uint16_t index = STORAGE_DATA_SIZE;
        while (index) {
            index--;
            if (handle->read_buffer.block.data[index] != '\0') {
                handle->read_buffer.index = index;

                break; // locate first non-zero byte from end of block
            }
        }
    }

    if (index < 0) {
        error = NVM_ERASED; // block is erased
    } else if (index < STORAGE_DATA_SIZE) {
        handle->read_buffer.index = index + 1;
        error = NVM_OK; // block is valid
    } else {
        error = NVM_FAIL;
    }

    uint8_t *block = (uint8_t *)&handle->read_buffer.block + STORAGE_BLOCK_SIZE - 1;
    do {
        if (*block != '\0') {
            break; // locate first non-zero byte from end of block
            block -= 1;
        }
    } while (block >=
             (uint8_t *)&handle->read_buffer.block) if (block <
                                                        (uint8_t *)&handle->read_buffer.block) {
        error = NVM_ERASED; // block is erased
    }
    else if (block > (uint8_t *)&handle->read_buffer.block.data) {
        handle->read_buffer.index = 0;
        error = NVM_FAIL; // block is valid
    }
    else {
        handle->read_buffer.index = block - (uint8_t *)&handle->read_buffer.block.data + 1;
    }

    for (uint16_t i = 0; i < STORAGE_BLOCK_SIZE; i++) {
        if (*block++ != handle->device->erased_value) {
            error = NVM_FAIL; // not erased
            break;
        }
    }

    if (error == NVM_FAIL) {
        error = NVM_ERASED; // finally check if the block is erased
        uint8_t *block = (uint8_t *)&handle->read_buffer.block;
        for (uint16_t i = 0; i < STORAGE_BLOCK_SIZE; i++) {
            if (*block++ != handle->device->erased_value) {
                error = NVM_FAIL; // not erased
                break;
            }
        }
    }
}
if (handle->read_block_index) {
    handle->read_block_index -= 1;
}
return error;
}

nvm_err_t storage_init(storage_handle_t *handle, nvm_device_t *device) {
    nvm_err_t error = NVM_OK;
    *handle = &storage_ctx;
    (*handle)->device = device;
    (*handle)->device->init();
    (*handle)->mutex = xSemaphoreCreateMutex();

    (*handle)->read_block_index = 0; // check if media is formatted
    if (storage_read_block(*handle) != NVM_OK) {
        storage_format(*handle);
    }

    uint32_t low_block_index = 1; // binary search for the first unused block
    uint32_t high_block_index = (*handle)->device->sector_count - 1;
    while (low_block_index != high_block_index) {
        uint32_t mid_block_index = low_block_index + (high_block_index - low_block_index) / 2;
        (*handle)->read_block_index = mid_block_index;
        if (storage_read_block(*handle) == NVM_OK) {
            low_block_index = mid_block_index + 1;
        } else {
            high_block_index = mid_block_index;
        }
    }
    (*handle)->write_block_index = low_block_index; // initialise read and write block indexes
    (*handle)->read_block_index = low_block_index - 1;
    return error;
}

nvm_err_t storage_format(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    if (xSemaphoreTake(handle->mutex, STORAGE_TIMEOUT) == pdTRUE) {
        error = handle->device->erase(0, handle->device->sector_count);
        if (error == NVM_OK) {
            handle->write_block_index = 0;
            memset(handle->write_buffer.block.data, 0, STORAGE_DATA_SIZE);
            error = storage_write_block(handle); // write first block with zeros
        }
        xSemaphoreGive(handle->mutex);
    } else {
        error = NVM_FAIL;
    }
    return error;
}

nvm_err_t storage_read_sync(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    if (xSemaphoreTake(handle->mutex, STORAGE_TIMEOUT) == pdTRUE) {
        handle->read_block_index = handle->write_block_index - 1;
        memcpy(&handle->read_buffer, &handle->write_buffer, sizeof(handle->read_buffer));
        xSemaphoreGive(handle->mutex);
    } else {
        error = NVM_FAIL;
    }
    return error;
}

nvm_err_t storage_write_sync(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    if (xSemaphoreTake(handle->mutex, STORAGE_TIMEOUT) == pdTRUE) {
        error = storage_write_block(handle);
        xSemaphoreGive(handle->mutex);
    } else {
        error = NVM_FAIL;
    }
    return error;
}

nvm_err_t storage_read_string(storage_handle_t handle, char *string, size_t maxlen) {
    nvm_err_t error = NVM_OK;
    if (xSemaphoreTake(handle->mutex, STORAGE_TIMEOUT) == pdTRUE) {

        if (handle->read_buffer.index == 0) {
            error = storage_read_block(handle);
        }

        handle->read_block_index = handle->write_block_index - 1;
        memcpy(&handle->read_buffer, &handle->write_buffer, sizeof(handle->read_buffer));
        xSemaphoreGive(handle->mutex);
    } else {
        error = NVM_FAIL;
    }
    return error;
}

nvm_err_t storage_write_string(storage_handle_t handle, const char *string) {
    nvm_err_t error = NVM_OK;
    uint16_t size = strlen(string) + 1; // we want to include the terminating null character
    if (size + handle->write_buffer.index > STORAGE_DATA_SIZE) {
        error = storage_write_block(handle);
    }
    memcpy(&handle->write_buffer.block.data[handle->write_buffer.index], s, size);
    handle->write_buffer.index += size;
    return error;
}
