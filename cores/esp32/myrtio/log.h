#pragma once

#include <stdint.h>
#include <stddef.h>

enum {
    LOG_NONE    = 0,
    LOG_ERROR   = 1,
    LOG_WARN    = 2,
    LOG_INFO    = 3,
    LOG_DEBUG   = 4,
    LOG_VERBOSE = 5,
};

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

int  log_printf(const char *fmt, ...);
void log_print_buf(const uint8_t *buf, size_t len);

// Compile-time level filtering: macros expand to log_printf when enabled
#define log_e(tag, fmt, ...) \
    do { if (LOG_LEVEL >= LOG_ERROR)   log_printf("E [%s] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
#define log_w(tag, fmt, ...) \
    do { if (LOG_LEVEL >= LOG_WARN)    log_printf("W [%s] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
#define log_i(tag, fmt, ...) \
    do { if (LOG_LEVEL >= LOG_INFO)    log_printf("I [%s] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
#define log_d(tag, fmt, ...) \
    do { if (LOG_LEVEL >= LOG_DEBUG)   log_printf("D [%s] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
#define log_v(tag, fmt, ...) \
    do { if (LOG_LEVEL >= LOG_VERBOSE) log_printf("V [%s] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
