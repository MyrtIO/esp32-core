#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MYRTIO_VERSION_MAJOR 0
#define MYRTIO_VERSION_MINOR 1
#define MYRTIO_VERSION_PATCH 0

// ---- Error codes ----
typedef int err_t;

enum {
    E_OK      =  0,
    E_FAIL    = -1,
    E_TIMEOUT = -2,
    E_ARG     = -3,
    E_STATE   = -4,
    E_NOMEM   = -5,
};

// ---- Pin identifier ----
typedef uint8_t pin_t;

// ---- Time value types ----
typedef uint32_t ms_t;
typedef uint64_t us_t;

// ---- Generic callback ----
typedef void (*callback_t)(void *arg);

// ---- Utility macros ----
#define MIN(a, b)         ((a) < (b) ? (a) : (b))
#define MAX(a, b)         ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)  (MIN(MAX(x, lo), hi))
#define ARRAY_SIZE(arr)   (sizeof(arr) / sizeof((arr)[0]))
#ifndef BIT
#define BIT(n)            (1UL << (n))
#endif
#define BIT_SET(x, n)     ((x) |= BIT(n))
#define BIT_CLEAR(x, n)   ((x) &= ~BIT(n))
#define BIT_READ(x, n)    (((x) >> (n)) & 1)
#define UNUSED(x)         ((void)(x))

// ---- Entry point ----
// User implements this. Called once from a FreeRTOS task.
#ifdef __cplusplus
extern "C" {
#endif

void app_start(void);

#ifdef __cplusplus
}
#endif
