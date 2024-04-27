#ifndef BATMON_LITTLEFS_H
#define BATMON_LITTLEFS_H

#define BATMON_LITTLEFS_BASE_PATH "/littlefs"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t batmon_littlefs_mount_sdspi(void);
void batmon_littlefs_init(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* BATMON_LITTLEFS_H_ */


