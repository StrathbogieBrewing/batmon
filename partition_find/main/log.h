#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

        #define LOG_DO_STRINGIFY(x) #x
        #define LOG_STRINGIFY(s) LOG_DO_STRINGIFY(s)

#define LOG_ERROR(fmt, ...) fprintf(stderr, "[ERROR "__FILE__ ":" LOG_STRINGIFY(__LINE__) "] \t" fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) fprintf(stderr, "[DEBUG "__FILE__ ":" LOG_STRINGIFY(__LINE__) "] \t" fmt "\n", ##__VA_ARGS__)
// #define LOG_DEBUG

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LOG_H_ */
