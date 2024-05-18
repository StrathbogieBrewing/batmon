#ifndef STORAGE_H
#define STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "nvm.h"

typedef struct storage_ctx_t storage_ctx_t;
typedef storage_ctx_t *storage_handle_t;

nvm_err_t storage_open(storage_handle_t *handle, nvm_device_t *device);
nvm_err_t storage_read_sync(storage_handle_t handle);
nvm_err_t storage_read_string(storage_handle_t handle, char *string, size_t maxlen);
nvm_err_t storage_write_string(storage_handle_t handle, const char *string);
nvm_err_t storage_write_sync(storage_handle_t handle);
nvm_err_t storage_format(storage_handle_t handle);
nvm_err_t storage_close(storage_handle_t handle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* STORAGE_H_ */
