#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "nvm_file.h"

#define NVM_FILE_SECTOR_COUNT 8
#define NVM_FILE_IMAGE_NAME "nvm_file_data.bin"

static nvm_err_t nvm_file_open(void);
static nvm_err_t nvm_file_read(uint32_t sector_index, uint8_t *sector_buffer);
static nvm_err_t nvm_file_write(uint32_t sector_index, uint8_t *sector_buffer);
static nvm_err_t nvm_file_erase(uint32_t sector_index, uint32_t sector_count);
static nvm_err_t nvm_file_close(void);

nvm_device_t nvm_file = {
    .open = nvm_file_open,
    .read = nvm_file_read,
    .write = nvm_file_write,
    .erase = nvm_file_erase,
    .close = nvm_file_close,
    .sector_size = NVM_SECTOR_SIZE,
    .sector_count = NVM_FILE_SECTOR_COUNT,
    .erase_count = NVM_SECTOR_SIZE,
    .erased_value = 0xFF,
};

static uint8_t nvm_file_data[NVM_SECTOR_SIZE * NVM_FILE_SECTOR_COUNT];

static nvm_err_t nvm_file_open(void) {
    nvm_err_t error = NVM_OK;
    FILE *fd = fopen(NVM_FILE_IMAGE_NAME, "r");
    if (fd == NULL) {
        LOG_ERROR("failed to open file %s", NVM_FILE_IMAGE_NAME);
        error = NVM_FAIL;
    } else {
        size_t read_size = fread(nvm_file_data, 1, sizeof(nvm_file_data), fd);
        if (read_size != sizeof(nvm_file_data)) {
            LOG_ERROR("failed to read file %s", NVM_FILE_IMAGE_NAME);
            error = NVM_FAIL;
        }
        fclose(fd);
    }
    if(error != NVM_OK) {
        memset(nvm_file_data, nvm_file.erased_value, sizeof(nvm_file_data));
    }
    return error;
}

static nvm_err_t nvm_file_read(uint32_t sector_index, uint8_t *sector_buffer) {
    nvm_err_t error = NVM_OK;
    if (sector_index >= NVM_FILE_SECTOR_COUNT) {
        LOG_ERROR("sector out of range %d", sector_index);
        error = NVM_FAIL;
    } else {
        memcpy(sector_buffer, &nvm_file_data[sector_index * NVM_SECTOR_SIZE],
               NVM_SECTOR_SIZE);
    }
    return error;
}

static nvm_err_t nvm_file_write(uint32_t sector_index, uint8_t *sector_buffer) {
    nvm_err_t error = NVM_OK;
    if (sector_index >= NVM_FILE_SECTOR_COUNT) {
        LOG_ERROR("sector out of range %d", sector_index);
        error = NVM_FAIL;
    } else {
        memcpy(&nvm_file_data[sector_index * NVM_SECTOR_SIZE], sector_buffer,
               NVM_SECTOR_SIZE);
    }
    return error;
}

static nvm_err_t nvm_file_erase(uint32_t sector_index, uint32_t sector_count) {
    nvm_err_t error = NVM_OK;
    if (sector_index + sector_count > NVM_FILE_SECTOR_COUNT) {
        LOG_ERROR("sectors out of range %d", sector_index);
        error = NVM_FAIL;
    } else {
        memset(&nvm_file_data[sector_index * NVM_SECTOR_SIZE], nvm_file.erased_value,
               sector_count * NVM_SECTOR_SIZE);
    }
    return error;
}

static nvm_err_t nvm_file_close(void) {
    nvm_err_t error = NVM_OK;
    FILE *fd = fopen(NVM_FILE_IMAGE_NAME, "w");
    if (fd == NULL) {
        LOG_ERROR("failed to create file %s", NVM_FILE_IMAGE_NAME);
        error = NVM_FAIL;
    } else {
        size_t write_size = fwrite(nvm_file_data, 1, sizeof(nvm_file_data), fd);
        if (write_size != sizeof(nvm_file_data)) {
            LOG_ERROR("failed to write file %s", NVM_FILE_IMAGE_NAME);
            error = NVM_FAIL;
        }
        fclose(fd);
    }
    return NVM_OK;
}
