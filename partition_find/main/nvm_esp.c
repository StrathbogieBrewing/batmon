#include <assert.h>
#include <string.h>

#include "esp_log.h"
#include "esp_partition.h"

// #include "spi_flash_chip_driver.h"
#include "spi_flash_mmap.h"

#include "nvm.h"

// #define NVM_ESP_SECTOR_SIZE (1024 * 4)
#define NVM_ESP_SECTOR_SIZE SPI_FLASH_SEC_SIZE

// typedef struct nvm_esp_sector_t {
//     uint8_t data[NVM_ESP_SECTOR_SIZE];
// } nvm_sector_t;

#define NVM_PARTITION_TYPE 0x64
#define NVM_PARTITION_SUB_TYPE 0x00

static const char *TAG = "nvm";

static nvm_err_t nvm_esp_init(void);
static nvm_err_t nvm_esp_read(uint32_t sector_index, uint8_t *sector_buffer);
static nvm_err_t nvm_esp_write(uint32_t sector_index, uint8_t *sector_buffer);
static nvm_err_t nvm_esp_erase(uint32_t sector_index, uint32_t sector_count);

nvm_device_t nvm_esp = {
    .init = nvm_esp_init,
    .read = nvm_esp_read,
    .write = nvm_esp_write,
    .erase = nvm_esp_erase,
    .sector_size = NVM_ESP_SECTOR_SIZE,
    .sector_count = 0,
    .erase_count = NVM_ESP_SECTOR_SIZE,
    .erased_value = 0xff,
};

static const esp_partition_t *nvm_esp_partition = NULL;

static nvm_err_t nvm_esp_init(void) {
    nvm_esp_partition = esp_partition_find_first(NVM_PARTITION_TYPE, NVM_PARTITION_SUB_TYPE, "nvm");
    if (nvm_esp_partition == NULL) {
        ESP_LOGE(TAG, "nvm partition not found!");
        return NVM_FAIL;
    } else {
        ESP_LOGI(TAG, "found nvm partition '%s' at offset 0x%" PRIx32 " with size 0x%" PRIx32" with sector size 0x%x" , nvm_esp_partition->label,
                 nvm_esp_partition->address, nvm_esp_partition->size, NVM_ESP_SECTOR_SIZE);
    }
    nvm_esp.sector_count = nvm_esp_partition->size / NVM_ESP_SECTOR_SIZE;
    nvm_esp.erase_count = nvm_esp_partition->erase_size / NVM_ESP_SECTOR_SIZE;
    return NVM_OK;
}

static nvm_err_t nvm_esp_read(uint32_t sector_index, uint8_t *sector_buffer) {
    esp_err_t err = esp_partition_read_raw(nvm_esp_partition, sector_index * NVM_ESP_SECTOR_SIZE, sector_buffer, NVM_ESP_SECTOR_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read block %" PRIx32 " failed!", sector_index);
        return NVM_FAIL;
    } else {
        ESP_LOGI(TAG, "read block %" PRIx32 " completed", sector_index);
    }
    return NVM_OK;
}

static nvm_err_t nvm_esp_write(uint32_t sector_index, uint8_t *sector_buffer) {
    esp_err_t err = esp_partition_write_raw(nvm_esp_partition, sector_index * NVM_ESP_SECTOR_SIZE, sector_buffer, NVM_ESP_SECTOR_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "write block %" PRIx32 " failed!", sector_index);
        return NVM_FAIL;
    } else {
        ESP_LOGI(TAG, "write block %" PRIx32 " completed", sector_index);
    }
    return NVM_OK;
}

static nvm_err_t nvm_esp_erase(uint32_t sector_index, uint32_t sector_count) {
    esp_err_t err = esp_partition_erase_range(nvm_esp_partition, sector_index * NVM_ESP_SECTOR_SIZE, sector_count * NVM_ESP_SECTOR_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "erase %" PRIx32 " blocks from block %" PRIx32 " failed with error %d", sector_count, sector_index,
                 err);
        return NVM_FAIL;
    } else {
        ESP_LOGI(TAG, "erase %" PRIx32 " blocks from block %" PRIx32 " completed", sector_count, sector_index);
    }
    return NVM_OK;
}


