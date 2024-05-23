#ifndef NVM_FILE_H
#define NVM_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <nvm.h>

#define NVM_SECTOR_SIZE (4096)

extern nvm_device_t nvm_file;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NVM_FILE_H_ */
