#ifndef REST_SERVER_H
#define REST_SERVER_H

#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t start_rest_server(const char *base_path);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* REST_SERVER_H_ */

