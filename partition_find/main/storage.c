#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "nvm_file.h"
#include "storage.h"
#include <arpa/inet.h>

#define STORAGE_MAGIC (htonl(0xDEADBEEF))

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
            LOG_DEBUG("wrapping back to first block");
        }
        if (handle->device->read(handle->write_block_index,
                                 (uint8_t *)&handle->cache_buffer.block) != NVM_ERASED) {
            error = handle->device->erase(handle->write_block_index, 1);
            LOG_DEBUG("erased block %d before write", handle->write_block_index);
        }
        error = handle->device->write(handle->write_block_index,
                                      (uint8_t *)&handle->write_buffer.block);
        memset(&handle->write_buffer, 0, sizeof(handle->write_buffer));
        LOG_DEBUG("write block %d ok", handle->write_block_index);
        handle->write_block_index += 1;
    } else {
        LOG_DEBUG("nothing to write");
    }
    return error;
}

static nvm_err_t storage_read_block(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    // LOG_DEBUG("read block %d", handle->read_block_index);
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
    if (error == NVM_FAIL) { // test if block is erased
        error = NVM_ERASED;
        for (uint8_t *block = (uint8_t *)&handle->read_buffer.block;
             block < (uint8_t *)&handle->read_buffer.block + STORAGE_BLOCK_SIZE; block++) {
            if (*block != handle->device->erased_value) {
                error = NVM_FAIL; // not erased
                LOG_DEBUG("not erased at %ld : %d", block - (uint8_t *)&handle->read_buffer.block,
                          *block);
                break;
            }
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
    LOG_DEBUG("open");
    *handle = &storage_ctx;
    (*handle)->device = device;
    (*handle)->device->open();

    (*handle)->read_block_index = 0; // check if media is formatted by checking block 0
    if (storage_read_block(*handle) != NVM_OK) {
        storage_format(*handle);
    }
    uint32_t high_block_index = (*handle)->device->sector_count - 1;
    uint32_t low_block_index = 1; // binary search for the first unused block
    uint32_t counter = 0;
    (*handle)->read_block_index = low_block_index;
    if (storage_read_block(*handle) == NVM_OK) {
        LOG_DEBUG("read block %d ok", low_block_index);
        counter = (*handle)->read_buffer.block.header.counter;
    }
    while (low_block_index != high_block_index) {
        uint32_t mid_block_index = low_block_index + (high_block_index - low_block_index) / 2;
        (*handle)->read_block_index = mid_block_index;
        error = storage_read_block(*handle);
        if (error == NVM_OK) {
            LOG_DEBUG("read block %d ok", mid_block_index);
            if (counter < (*handle)->read_buffer.block.header.counter) {
                low_block_index = mid_block_index;
            } else {
                high_block_index = mid_block_index;
            }
            counter = (*handle)->read_buffer.block.header.counter;
        } else {
            high_block_index = mid_block_index; // must be first pass of formated media
            if (error == NVM_ERASED) {
                LOG_DEBUG("read block %d erased", mid_block_index);
            } else {
                LOG_DEBUG("read block %d failed", mid_block_index);
            }
        }
    }
    (*handle)->write_counter = (*handle)->read_buffer.block.header.counter + 1;
    LOG_DEBUG("write counter %d , counter %d", (*handle)->write_counter, counter);
    if ((*handle)->write_counter == 0) {
        low_block_index = 0;
    }
    (*handle)->read_block_index = low_block_index; // initialise read and write block indexes
    (*handle)->write_block_index = low_block_index + 1;
    LOG_DEBUG("read block index %d ", (*handle)->read_block_index);
    return error;
}

nvm_err_t storage_format(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    LOG_DEBUG("format");
    error = handle->device->erase(0, handle->device->sector_count);
    if (error == NVM_OK) {
        handle->write_block_index = 0;
        memset(&handle->write_buffer.block, 0, sizeof(handle->write_buffer.block));
        storage_write_string(handle, "NVM STRING LOGGER");

        storage_write_sync(handle);
    } else {
        LOG_ERROR("erase failed");
    }
    return error;
}

nvm_err_t storage_read_sync(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    // LOG_DEBUG("read sync");
    handle->read_block_index = handle->write_block_index - 1;
    memcpy(&handle->read_buffer, &handle->write_buffer, sizeof(handle->read_buffer));
    return error;
}

nvm_err_t storage_write_sync(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    // LOG_DEBUG("write sync");
    error = storage_write_block(handle);
    return error;
}

nvm_err_t storage_read_string(storage_handle_t handle, char *string, size_t maxlen) {
    nvm_err_t error = NVM_OK;
    // LOG_DEBUG("read string");

    if (handle->read_buffer.index == 0) {
        if(handle->read_block_index == handle->write_block_index) {
            error = NVM_EMPTY;
        } else {
            error = storage_read_block(handle); // read new data buffer
        }
        if (error == NVM_OK) {
            while ((handle->read_buffer.block.data[handle->read_buffer.index] == '\0') &&
                   (handle->read_buffer.index != 0)) {
                handle->read_buffer.index -= 1; // update index to end of last string in buffer
            }
            if (handle->read_buffer.index == 0) {
                LOG_DEBUG("read buffer empty");
                error = NVM_EMPTY;
            } else {
                // LOG_DEBUG("read buffer index %d", handle->read_buffer.index);
            }
        } else {
            LOG_DEBUG("read block failed");
        }
    }
    if ((error == NVM_OK) && (handle->read_buffer.index != 0)) {

        uint16_t end_index = handle->read_buffer.index - 1;  // this should be the null terminator
        uint16_t start_index = end_index - 1;  // this should be the last character of the string
        while ((handle->read_buffer.block.data[start_index] != '\0') && (start_index != 0)) {
            start_index -= 1; // step back through characters to the null before the beginning of the string
        }
        if(start_index != 0) {
            start_index += 1; // if its the null terminator, step forward to the start of the string
        }
        handle->read_buffer.index = start_index;
        // LOG_DEBUG("read string start : 0x%2.2X\tend : 0x%2.2X\tfrom 0x%4.4X", start_index, end_index, handle->read_block_index);
        if (end_index - start_index > maxlen) {
            error = NVM_FAIL;
        } else {
            // LOG_DEBUG("read string 0x%4.4X : 0x%2.2X : <%s>", handle->read_block_index, start_index, &handle->read_buffer.block.data[start_index]);
            strcpy(string, (char*)&handle->read_buffer.block.data[start_index]);
        }
    }
    return error;
}

nvm_err_t storage_write_string(storage_handle_t handle, const char *string) {
    nvm_err_t error = NVM_OK;
    // LOG_DEBUG("write string : %s", string);
    uint16_t size = strlen(string) + 1; // we want to include the terminating null character
    if (handle->write_buffer.index + size > STORAGE_DATA_SIZE) {
        error = storage_write_block(handle);
    }
    memcpy(&handle->write_buffer.block.data[handle->write_buffer.index], string, size);
    handle->write_buffer.index += size;
    return error;
}

nvm_err_t storage_close(storage_handle_t handle) {
    nvm_err_t error = NVM_OK;
    LOG_DEBUG("close");
    error = storage_write_sync(handle);
    handle->device->close();
    return error;
}