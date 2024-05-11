#ifndef SDLOG_H
#define SDLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SDLOG_BLOCK_SIZE (1024 * 4)
#define SDLOG_ERASE_SIZE SDLOG_WRITE_SIZE
#define SDLOG_FLASH_SIZE (SDLOG_ERASE_SIZE * 1024)

#define SDLOG_MAX_MSG_SIZE (256)

typedef struct sdlog_msg_t {
    uint8_t size;
    uint8_t data[SDLOG_MAX_MSG_SIZE];
} sdlog_msg_t;

typedef enum {
    SDLOG_OK = 0,
    SDLOG_FAIL,
} sdlog_err_t;

sdlog_err_t sdlog_init(void);
sdlog_err_t sdlog_read(sdlog_msg_t * msg);
sdlog_err_t sdlog_write(sdlog_msg_t * msg);
sdlog_err_t sdlog_erase(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* SDLOG_H_ */


