#ifndef NVM_H
#define NVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

typedef enum {
    NVM_OK = EXIT_SUCCESS,
    NVM_FAIL,
    NVM_FULL,
} nvm_err_t;

typedef nvm_err_t (*nvm_init)(void);
typedef nvm_err_t (*nvm_read)(uint32_t sector_index, uint8_t *sector_buffer);
typedef nvm_err_t (*nvm_write)(uint32_t sector_index, uint8_t *sector_buffer);
typedef nvm_err_t (*nvm_erase)(uint32_t sector_index, uint32_t sector_count);

typedef struct nvm_device_t {
    const nvm_init init;
    const nvm_read read;
    const nvm_write write;
    const nvm_erase erase;
    uint32_t sector_size;
    uint32_t sector_count;
    uint32_t erase_count;
} nvm_device_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NVM_H_ */
