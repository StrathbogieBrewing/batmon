#ifndef TINBUS_H
#define TINBUS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tinbus_msg_t {
    uint8_t device_id;
    int16_t value;
} tinbus_msg_t;

esp_err_t tinbus_init(void);

esp_err_t tinbus_read(tinbus_msg_t *msg);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* TINBUS_H */