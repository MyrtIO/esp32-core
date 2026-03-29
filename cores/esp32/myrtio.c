#include "myrtio.h"

#include "esp32-hal.h"
#include "esp32-hal-gpio.h"

#ifndef MYRTIO_DEFAULT_BUILTIN_LED
#define MYRTIO_DEFAULT_BUILTIN_LED 2
#endif

void myrtio_pin_mode(uint8_t pin, uint8_t mode) {
    pinMode(pin, mode);
}

void myrtio_digital_write(uint8_t pin, uint8_t value) {
    digitalWrite(pin, value);
}

void myrtio_delay_ms(uint32_t ms) {
    delay(ms);
}

uint8_t myrtio_builtin_led(void) {
#ifdef LED_BUILTIN
    return LED_BUILTIN;
#else
    return MYRTIO_DEFAULT_BUILTIN_LED;
#endif
}
