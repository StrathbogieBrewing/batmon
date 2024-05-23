#include <stdio.h>
#include <string.h>

#include "nvm_file.h"
#include "storage.h"

#include <sys/time.h>

#include <stdlib.h>
#include <time.h>

#define TEST_DATA_SIZE (1024 * 8)

static uint8_t random_data[TEST_DATA_SIZE] = {0};

void random_string(uint8_t *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] = (rand() & 0x0f) + 'A';
    }
    buffer[length] = '\0';
}

void init_test_data(void) {
    int data_index = 0;
    while (data_index < TEST_DATA_SIZE - nvm_file.sector_size) {
        uint8_t data_length =
            (rand() & 0x1f) + 1; // Returns a pseudo-random integer between 0 and 127.
        random_string(&random_data[data_index], data_length);
        data_index += data_length + 1;
    }
}

int main(int argc, char **argv) {
    storage_handle_t handle;

    srand(time(NULL)); // Initialization, should only be called once.

    struct timeval te;
    gettimeofday(&te, NULL); // get current time

    init_test_data();

    nvm_err_t error = storage_open(&handle, &nvm_file);

    int test_write_index = 0;

    while (test_write_index < TEST_DATA_SIZE - nvm_file.sector_size) {
        storage_write_string(handle, &random_data[test_write_index]);
        test_write_index += strlen((char *)&random_data[test_write_index]) + 1;

        int test_read_index = test_write_index - 2;
        storage_read_sync(handle);
        while (test_read_index != 0) {
            while ((random_data[test_read_index] != '\0') && (test_read_index != 0)) {
                test_read_index--;
            }
            if (test_read_index) {
                test_read_index += 1;
            }

            uint8_t read_buffer[nvm_file.sector_size];
            storage_read_string(handle, read_buffer, nvm_file.sector_size);
            if (strcmp((char *)read_buffer, (char *)&random_data[test_read_index]) != 0) {
                printf("Error: Data read does not match data written 0x%4.4X, 0x%4.4x\nread data : "
                       "<%s>\ntest data : <%s>\n",
                       test_read_index, test_write_index, (char *)read_buffer,
                       (char *)&random_data[test_read_index]);
                       goto exit;
            } else {
                // printf("Data read matches data written %d, %d\n<%s>\n<%s>\n", test_read_index,
                //        test_write_index, (char *)read_buffer,
                //        (char *)&random_data[test_read_index]);
            }
            if (test_read_index) {
                test_read_index -= 1;
            }
            if (test_read_index) {
                test_read_index -= 1;
            }
        }
    }

    // // storage_write_string(handle, "Test");
    // for (int i = 0; i < 16; i++) {

    //     uint8_t buffer[nvm_file.sector_size];
    //     sprintf(buffer, "I%3.3dT%5.5d     ", i, (uint16_t)te.tv_sec / 60);
    //     storage_write_string(handle, buffer);
    // }
    // storage_write_string(handle, "Test finished");

    // uint8_t sector_buffer[nvm_file.sector_size];
    // nvm_file.open();
    // nvm_file.read(0, sector_buffer);
    // strcpy((char *)sector_buffer, "Hello, World!");
    // nvm_file.write(0, sector_buffer);
    // nvm_file.close();
exit:
    error = storage_close(handle);
    return 0;
}