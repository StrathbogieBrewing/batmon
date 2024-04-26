#ifndef SNTP_CLIENT_H
#define SNTP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

void sntp_client_init(void);
bool sntp_time_is_set(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* SNTP_CLIENT_H_ */


