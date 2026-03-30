#include "sys.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"
#include "esp_idf_version.h"
#include "sdkconfig.h"

#include "esp_spi_flash.h"
#if CONFIG_SPIRAM_SUPPORT || CONFIG_SPIRAM
#include "esp32/spiram.h"
#endif

void sys_restart(void) {
    esp_restart();
}

void sys_deep_sleep(us_t time_us) {
    esp_deep_sleep(time_us);
}

const char *sys_chip_model(void) {
    esp_chip_info_t info;
    esp_chip_info(&info);
    switch (info.model) {
        case CHIP_ESP32:   return "ESP32";
        case CHIP_ESP32S2: return "ESP32-S2";
        case CHIP_ESP32S3: return "ESP32-S3";
        case CHIP_ESP32C3: return "ESP32-C3";
        default:           return "unknown";
    }
}

uint8_t sys_chip_revision(void) {
    esp_chip_info_t info;
    esp_chip_info(&info);
    return (uint8_t)info.revision;
}

uint8_t sys_chip_cores(void) {
    esp_chip_info_t info;
    esp_chip_info(&info);
    return (uint8_t)info.cores;
}

uint64_t sys_efuse_mac(void) {
    uint64_t mac = 0;
    esp_efuse_mac_get_default((uint8_t *)&mac);
    return mac;
}

float sys_temperature(void) {
#ifdef CONFIG_IDF_TARGET_ESP32
    extern uint8_t temprature_sens_read(void);
    return (temprature_sens_read() - 32) / 1.8f;
#else
    return 0.0f;
#endif
}

uint32_t sys_heap_total(void) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    return (uint32_t)(info.total_free_bytes + info.total_allocated_bytes);
}

uint32_t sys_heap_free(void) {
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t sys_heap_min_free(void) {
    return (uint32_t)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
}

uint32_t sys_heap_max_alloc(void) {
    return (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
}

bool sys_psram_found(void) {
#if CONFIG_SPIRAM_SUPPORT || CONFIG_SPIRAM
    return esp_spiram_is_initialized() == true;
#else
    return false;
#endif
}

uint32_t sys_psram_total(void) {
    if (!sys_psram_found()) {
        return 0;
    }
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    return (uint32_t)(info.total_free_bytes + info.total_allocated_bytes);
}

uint32_t sys_psram_free(void) {
    if (!sys_psram_found()) {
        return 0;
    }
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

void *sys_psram_malloc(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void *sys_psram_calloc(size_t n, size_t size) {
    return heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void *sys_psram_realloc(void *ptr, size_t size) {
    return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

uint32_t sys_flash_size(void) {
    return (uint32_t)spi_flash_get_chip_size();
}

const char *sys_sdk_version(void) {
    return esp_get_idf_version();
}

void sys_wdt_enable(void) {
    esp_task_wdt_init(5, true);
    esp_task_wdt_add(NULL);
}

void sys_wdt_disable(void) {
    esp_task_wdt_delete(NULL);
    esp_task_wdt_deinit();
}

void sys_wdt_feed(void) {
    esp_task_wdt_reset();
}
