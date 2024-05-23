#ifndef NVM_ESP_H
#define NVM_ESP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <nvm.h>

#define NVM_SECTOR_SIZE (4096)

extern nvm_device_t nvm_esp;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NVM_ESP_H_ */
