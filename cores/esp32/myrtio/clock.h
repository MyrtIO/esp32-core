#pragma once
#include "core.h"

// ---- Delays ----
void delay_ms(ms_t ms);
void delay_us(us_t us);

// ---- Uptime ----
ms_t uptime_ms(void);
us_t uptime_us(void);

// ---- Yield ----
void yield(void);

// ---- CPU frequency ----
bool     clock_set_cpu_mhz(uint32_t mhz);
uint32_t clock_get_cpu_mhz(void);
uint32_t clock_get_xtal_mhz(void);
uint32_t clock_get_apb_hz(void);

// ---- APB change notifications ----
typedef enum {
    CLOCK_BEFORE_CHANGE,
    CLOCK_AFTER_CHANGE,
} clock_event_t;

typedef void (*clock_change_cb_t)(void *arg, clock_event_t event,
                                  uint32_t old_apb, uint32_t new_apb);

bool clock_on_apb_change(clock_change_cb_t cb, void *arg);
bool clock_off_apb_change(clock_change_cb_t cb, void *arg);
