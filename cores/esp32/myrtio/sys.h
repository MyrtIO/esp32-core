#pragma once
#include "core.h"

// ---- Lifecycle ----
void sys_restart(void);
void sys_deep_sleep(us_t time_us);

// ---- Chip info ----
const char *sys_chip_model(void);
uint8_t     sys_chip_revision(void);
uint8_t     sys_chip_cores(void);
uint64_t    sys_efuse_mac(void);

// ---- Temperature ----
float sys_temperature(void);

// ---- Internal heap ----
uint32_t sys_heap_total(void);
uint32_t sys_heap_free(void);
uint32_t sys_heap_min_free(void);
uint32_t sys_heap_max_alloc(void);

// ---- PSRAM ----
bool     sys_psram_found(void);
uint32_t sys_psram_total(void);
uint32_t sys_psram_free(void);
void    *sys_psram_malloc(size_t size);
void    *sys_psram_calloc(size_t n, size_t size);
void    *sys_psram_realloc(void *ptr, size_t size);

// ---- Flash ----
uint32_t sys_flash_size(void);

// ---- SDK version ----
const char *sys_sdk_version(void);

// ---- WDT ----
void sys_wdt_enable(void);
void sys_wdt_disable(void);
void sys_wdt_feed(void);
