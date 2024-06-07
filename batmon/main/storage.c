#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "nvm_file.h"
#include "storage.h"
#include <arpa/inet.h>

#define STORAGE_MAGIC (htonl(0xDEADBEEF))

static const char *TAG = "storage";

typedef struct storage_header_t {
    uint32_t magic;
    uint32_t counter;
    uint32_t crc;
    uint32_t flags;
} storage_header_t;

#define STORAGE_BLOCK_SIZE NVM_SECTOR_SIZE
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
    nvm_device_t *device;
    storage_buffer_t read_buffer;
    storage_buffer_t write_buffer;
    storage_buffer_t cache_buffer;
    uint32_t read_block_index;  // next block to read
    uint32_t write_block_index; // next block to be written
    uint32_t write_counter;
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
    if (handle->write_buffer.index != 0) {
        handle->write_buffer.block.header.magic = STORAGE_MAGIC;
        handle->write_buffer.block.header.counter = handle->write_counter++;
        handle->write_buffer.block.header.crc =
            storage_crc16(handle->write_buffer.block.data, STORAGE_DATA_SIZE);
        if (handle->write_block_index >= handle->device->sector_count) {
            handle->write_block_index = 1; // wrap to beginning of media
        }
        if (handle->device->read(handle->write_block_index,
                                 (uint8_t *)&handle->cache_buffer.block) != NVM_ERASED) {
            error = handle->device->erase(handle->write_block_index, 1);
        }
        error = handle->device->write(handle->write_block_index,
                                      (uint8_t *)&handle->write_buffer.block);
        memset(&handle->write_buffer, 0, sizeof(handle->write_buffer));
        handle->write_block_index += 1;
    } else {
        LOG_ERROR(TAG, "nothing to write");
    }
    return error;
}

static nvm_err_t storage_read_block(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    error = handle->device->read(handle->read_block_index, (uint8_t *)&handle->read_buffer.block);
    handle->read_buffer.index = STORAGE_DATA_SIZE - 1; // start reading from end of block
    if (error == NVM_OK) {                             // test if block is valid
        if (handle->read_buffer.block.header.magic != STORAGE_MAGIC) {
            error = NVM_FAIL; // wrong magic number
        }
        if (handle->read_buffer.block.header.crc !=
            storage_crc16(handle->read_buffer.block.data, STORAGE_DATA_SIZE)) {
            error = NVM_FAIL; // bad crc
        }
    }
    if (handle->read_block_index > 1) {
        handle->read_block_index -= 1; // advance to next read block
    } else {
        handle->read_block_index = handle->device->sector_count - 1; // wrap to last block
    }
    return error;
}

nvm_err_t storage_open(storage_handle_t *handle, nvm_device_t *device) {
    nvm_err_t error = NVM_OK;
    *handle = &storage_ctx;
    (*handle)->device = device;
    if((*handle)->device->open() != NVM_OK) {
        LOG_ERROR(TAG, "open failed");
        return NVM_FAIL;
    }
    (*handle)->read_block_index = 0; // check if media is formatted by checking block 0
    if (storage_read_block(*handle) != NVM_OK) {
        // storage_format(*handle);
    }
    // uint32_t high_block_index = (*handle)->device->sector_count - 1;
    // uint32_t low_block_index = 1; // binary search for the first unused block
    // uint32_t counter = 0;
    // (*handle)->read_block_index = low_block_index;
    // if (storage_read_block(*handle) == NVM_OK) {
    //     counter = (*handle)->read_buffer.block.header.counter;
    // }
    // while (low_block_index != high_block_index) {
    //     uint32_t mid_block_index = low_block_index + (high_block_index - low_block_index) / 2;
    //     (*handle)->read_block_index = mid_block_index;
    //     error = storage_read_block(*handle);
    //     if (error == NVM_OK) {
    //         if (counter < (*handle)->read_buffer.block.header.counter) {
    //             low_block_index = mid_block_index;
    //         } else {
    //             high_block_index = mid_block_index;
    //         }
    //         counter = (*handle)->read_buffer.block.header.counter;
    //     } else {
    //         high_block_index = mid_block_index; // must be first pass of formated media
    //         if (error != NVM_ERASED) {
    //             LOG_ERROR(TAG, "read block %d failed", (int)mid_block_index);
    //         }
    //     }
    // }
    // (*handle)->write_counter = (*handle)->read_buffer.block.header.counter + 1;
    // if ((*handle)->write_counter == 0) {
    //     low_block_index = 0;
    // }
    // (*handle)->read_block_index = low_block_index; // initialise read and write block indexes
    // (*handle)->write_block_index = low_block_index + 1;
    return error;
}

nvm_err_t storage_format(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    error = handle->device->erase(0, handle->device->sector_count);
    if (error == NVM_OK) {
        handle->write_block_index = 0;
        memset(&handle->write_buffer.block, 0, sizeof(handle->write_buffer.block));
        storage_write_string(handle, "NVM STRING LOGGER");
        storage_write_sync(handle);
    } else {
        LOG_ERROR(TAG, "erase failed");
    }
    return error;
}

nvm_err_t storage_read_sync(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    handle->read_block_index = handle->write_block_index - 1;
    memcpy(&handle->read_buffer, &handle->write_buffer, sizeof(handle->read_buffer));
    if (handle->read_buffer.index != 0) {
        handle->read_buffer.index -= 1; // step back to the null terminator
    }
    return error;
}

nvm_err_t storage_write_sync(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    error = storage_write_block(handle);
    return error;
}

nvm_err_t storage_read_string(storage_handle_t handle, char *string, size_t maxlen) {
    nvm_err_t error = NVM_OK;
    if (handle->read_buffer.index == 0) {
        if (handle->read_block_index == handle->write_block_index) {
            error = NVM_EMPTY;
        } else {
            error = storage_read_block(handle); // read new data buffer
        }
        if (error == NVM_OK) {
            while ((handle->read_buffer.block.data[handle->read_buffer.index] == '\0') &&
                   (handle->read_buffer.index != 0)) {
                handle->read_buffer.index -= 1; // update index to end of last character in buffer
            }
            if (handle->read_buffer.index == 0) {
                LOG_ERROR(TAG, "read buffer empty");
                error = NVM_EMPTY;
            } else {
                handle->read_buffer.index += 1; // step forward to the null terminator
            }
        } else {
            LOG_ERROR(TAG, "read block failed");
        }
    }
    if ((error == NVM_OK) && (handle->read_buffer.index != 0)) {
        uint16_t end_index = handle->read_buffer.index; // this should be the null terminator
        uint16_t start_index = end_index - 1; // this should be the last character of the string
        while ((handle->read_buffer.block.data[start_index] != '\0') && (start_index != 0)) {
            start_index -= 1; // step back to the null before the beginning of the string
        }
        handle->read_buffer.index = start_index;
        if (end_index - start_index > maxlen) {
            error = NVM_FAIL;
        } else {
            if (start_index != 0) {
                start_index += 1; // step forward to the start of the string
            }
            strcpy(string, (char *)&handle->read_buffer.block.data[start_index]);
        }
    }
    return error;
}

nvm_err_t storage_write_string(storage_handle_t handle, const char *string) {
    nvm_err_t error = NVM_OK;
    uint16_t size = strlen(string) + 1; // we also want to write the terminating null character
    if (handle->write_buffer.index + size > STORAGE_DATA_SIZE) {
        error = storage_write_block(handle);
    }
    memcpy(&handle->write_buffer.block.data[handle->write_buffer.index], string, size);
    handle->write_buffer.index += size;
    return error;
}

nvm_err_t storage_close(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    error = storage_write_sync(handle);
    handle->device->close();
    return error;
}