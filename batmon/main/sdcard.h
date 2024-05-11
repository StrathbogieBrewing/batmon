#ifndef SDCARD_H
#define SDCARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SDCARD_BLOCK_SIZE (1024 * 4)

typedef uint8_t sdcard_data_t[SDCARD_BLOCK_SIZE];

typedef enum {
    SDCARD_OK = 0,
    SDCARD_FAIL,
} sdcard_err_t;

sdcard_err_t sdcard_init(void);
sdcard_err_t sdcard_read(uint32_t block, sdcard_data_t *data);
sdcard_err_t sdcard_write(uint32_t block, sdcard_data_t *data);
sdcard_err_t sdcard_erase(uint32_t start_block, uint32_t quantity);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* SDCARD_H_ */
