#include <string.h>
#include <stdio.h>

#include "nvm_file.h"
#include "storage.h"

int main(int argc, char **argv) {
    storage_handle_t handle;
    nvm_err_t error = storage_open(&handle, &nvm_file);

    storage_write_string(handle, "Test started");
    for(int i = 0; i < 16; i++) {
        uint8_t buffer[nvm_file.sector_size];
        sprintf( buffer, "String test id: %d", i);
        storage_write_string(handle, buffer);
    }
    storage_write_string(handle, "Test finished");

    // uint8_t sector_buffer[nvm_file.sector_size]; 
    // nvm_file.open();
    // nvm_file.read(0, sector_buffer);
    // strcpy((char *)sector_buffer, "Hello, World!");
    // nvm_file.write(0, sector_buffer);
    // nvm_file.close();

    error = storage_close(handle);
    return 0;
}