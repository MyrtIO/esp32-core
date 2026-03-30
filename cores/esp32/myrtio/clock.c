#include "clock.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "soc/rtc.h"

void delay_ms(ms_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void delay_us(us_t us) {
    uint64_t end = (uint64_t)esp_timer_get_time() + us;
    while ((uint64_t)esp_timer_get_time() < end) {
        __asm__ __volatile__("nop");
    }
}

ms_t uptime_ms(void) {
    return (ms_t)(esp_timer_get_time() / 1000ULL);
}

us_t uptime_us(void) {
    return (us_t)esp_timer_get_time();
}

void yield(void) {
    taskYIELD();
}

bool clock_set_cpu_mhz(uint32_t mhz) {
    rtc_cpu_freq_config_t conf;
    if (!rtc_clk_cpu_freq_mhz_to_config(mhz, &conf)) {
        return false;
    }
    rtc_clk_cpu_freq_set_config(&conf);
    return true;
}

uint32_t clock_get_cpu_mhz(void) {
    rtc_cpu_freq_config_t conf;
    rtc_clk_cpu_freq_get_config(&conf);
    return conf.freq_mhz;
}

uint32_t clock_get_xtal_mhz(void) {
    return (uint32_t)rtc_clk_xtal_freq_get();
}

uint32_t clock_get_apb_hz(void) {
    return rtc_clk_apb_freq_get();
}

bool clock_on_apb_change(clock_change_cb_t cb, void *arg) {
    UNUSED(cb);
    UNUSED(arg);
    return false;
}

bool clock_off_apb_change(clock_change_cb_t cb, void *arg) {
    UNUSED(cb);
    UNUSED(arg);
    return false;
}
