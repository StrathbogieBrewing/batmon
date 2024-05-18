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
    NVM_ERASED,
    NVM_EMPTY,
} nvm_err_t;

typedef nvm_err_t (*nvm_open)(void);
typedef nvm_err_t (*nvm_read)(uint32_t sector_index, uint8_t *sector_buffer);
typedef nvm_err_t (*nvm_write)(uint32_t sector_index, uint8_t *sector_buffer);
typedef nvm_err_t (*nvm_erase)(uint32_t sector_index, uint32_t sector_count);
typedef nvm_err_t (*nvm_close)(void);

typedef struct nvm_device_t {
    const nvm_open open;
    const nvm_read read;
    const nvm_write write;
    const nvm_erase erase;
    const nvm_close close;
    uint32_t sector_size;
    uint32_t sector_count;
    uint32_t erase_count;
    uint8_t erased_value;
} nvm_device_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NVM_H_ */
