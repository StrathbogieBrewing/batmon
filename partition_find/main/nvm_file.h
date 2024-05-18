#ifndef NVM_FILE_H
#define NVM_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <nvm.h>

#define NVM_FILE_SECTOR_SIZE (256)
#define NVM_FILE_SECTOR_COUNT 16
// #define NVM_FILE_SECTOR_SIZE (1024 * 4)
// #define NVM_FILE_SECTOR_COUNT 16
#define NVM_FILE_IMAGE_NAME "nvm_file_data.bin"

extern nvm_device_t nvm_file;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NVM_FILE_H_ */
