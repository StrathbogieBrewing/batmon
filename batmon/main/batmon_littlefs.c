#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_system.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"

#include "batmon_littlefs.h"
#include "hardware.h"

static const char *TAG = "batmon_littlefs";
void batmon_littlefs_test(void) {
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE *f = fopen("/littlefs/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello World!\n");
    fclose(f);
    ESP_LOGI(TAG, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/littlefs/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/littlefs/foo.txt");
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file");
    if (rename("/littlefs/hello.txt", "/littlefs/foo.txt") != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file");
    f = fopen("/littlefs/foo.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    char line[128];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    ESP_LOGI(TAG, "Reading from flashed filesystem example.txt");
    f = fopen("/littlefs/example.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);


}

esp_err_t batmon_littlefs_mount_sdspi(void) {
    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host_config = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = HARDWARE_SDCARD_MOSI,
        .miso_io_num = HARDWARE_SDCARD_MISO,
        .sclk_io_num = HARDWARE_SDCARD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(host_config.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = HARDWARE_SDCARD_CS;
    slot_config.host_id = host_config.slot;

    static sdmmc_card_t sdcard = {0};

    (host_config.init)();
    int card_handle = -1; // uninitialized

    ESP_ERROR_CHECK(sdspi_host_init_device((const sdspi_device_config_t *)&slot_config, &card_handle));

    // init_sdspi_host(host_config.slot, &slot_config, &card_handle);

    host_config.slot = card_handle;

    // probe and initialize card
    ret = sdmmc_card_init(&host_config, &sdcard);

    // ret = esp_vfs_fat_sdspi_mount(base_path, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG,
                     "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG,
                     "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return ret;
    }

    esp_vfs_littlefs_conf_t conf = {
        .base_path = BATMON_LITTLEFS_BASE_PATH,
        .partition_label = NULL,
        .partition = NULL,
        .format_if_mount_failed = true,
        .dont_mount = false,

        .sdcard = &sdcard,
    };

    ret = esp_vfs_littlefs_register(&conf);

    sdmmc_card_print_info(stdout, &sdcard);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    // size_t total = 0, used = 0;
    // ret = esp_littlefs_info(conf.partition_label, &total, &used);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    //     esp_littlefs_format(conf.partition_label);
    // } else {
    //     ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    // }

    batmon_littlefs_test();

    return ESP_OK;
}

void batmon_littlefs_init(void) {
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = BATMON_LITTLEFS_BASE_PATH,
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        esp_littlefs_format(conf.partition_label);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // // Use POSIX and C standard library functions to work with files.
    // // First create a file.
    // ESP_LOGI(TAG, "Opening file");
    // FILE *f = fopen("/littlefs/hello.txt", "w");
    // if (f == NULL) {
    //     ESP_LOGE(TAG, "Failed to open file for writing");
    //     return;
    // }
    // fprintf(f, "Hello World!\n");
    // fclose(f);
    // ESP_LOGI(TAG, "File written");

    // // Check if destination file exists before renaming
    // struct stat st;
    // if (stat("/littlefs/foo.txt", &st) == 0) {
    //     // Delete it if it exists
    //     unlink("/littlefs/foo.txt");
    // }

    // // Rename original file
    // ESP_LOGI(TAG, "Renaming file");
    // if (rename("/littlefs/hello.txt", "/littlefs/foo.txt") != 0) {
    //     ESP_LOGE(TAG, "Rename failed");
    //     return;
    // }

    // // Open renamed file for reading
    // ESP_LOGI(TAG, "Reading file");
    // f = fopen("/littlefs/foo.txt", "r");
    // if (f == NULL) {
    //     ESP_LOGE(TAG, "Failed to open file for reading");
    //     return;
    // }

    // char line[128];
    // fgets(line, sizeof(line), f);
    // fclose(f);
    // // strip newline
    // char*pos = strchr(line, '\n');
    // if (pos) {
    //     *pos = '\0';
    // }
    // ESP_LOGI(TAG, "Read from file: '%s'", line);

    // ESP_LOGI(TAG, "Reading from flashed filesystem example.txt");
    // f = fopen("/littlefs/example.txt", "r");
    // if (f == NULL) {
    //     ESP_LOGE(TAG, "Failed to open file for reading");
    //     return;
    // }
    // fgets(line, sizeof(line), f);
    // fclose(f);
    // // strip newline
    // pos = strchr(line, '\n');
    // if (pos) {
    //     *pos = '\0';
    // }
    // ESP_LOGI(TAG, "Read from file: '%s'", line);

    // // All done, unmount partition and disable LittleFS
    // esp_vfs_littlefs_unregister(conf.partition_label);
    // ESP_LOGI(TAG, "LittleFS unmounted");
}
