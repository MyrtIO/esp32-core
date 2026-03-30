#ifndef MYRTIO_GPIO_H
#define MYRTIO_GPIO_H

#include <esp32-hal-gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t gpio_pin_t;
typedef uint8_t gpio_mode_t;
typedef uint8_t gpio_level_t;

enum {
    GPIO_MODE_INPUT = 0x01,
    GPIO_MODE_OUTPUT = 0x03,
    GPIO_MODE_PULLUP = 0x04,
    GPIO_MODE_INPUT_PULLUP = 0x05,
    GPIO_MODE_PULLDOWN = 0x08,
    GPIO_MODE_INPUT_PULLDOWN = 0x09,
    GPIO_MODE_OPEN_DRAIN = 0x10,
    GPIO_MODE_OUTPUT_OPEN_DRAIN = 0x13,
    GPIO_MODE_ANALOG = 0xC0,
} gpio_mode_t;

enum {
    GPIO_LEVEL_LOW = 0,
    GPIO_LEVEL_HIGH = 1,
} gpio_level_t;

void gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode) {
    pinMode(pin, mode);
}

void gpio_digital_write(gpio_pin_t pin, gpio_level_t value) {
    digitalWrite(pin, value);
}

gpio_level_t gpio_digital_read(gpio_pin_t pin) {
    return digitalRead(pin);
}

#ifdef LED_BUILTIN
#define MYRTIO_LED_BUILTIN LED_BUILTIN
#else
#define MYRTIO_LED_BUILTIN 2
#endif

// typedef void (*myrtio_gpio_isr_fn)(gpio_pin_t pin, void *user_ctx);

// void hw_gpio_attach_isr(gpio_pin_t pin, myrtio_gpio_isr_fn isr, void *user_ctx);
// void hw_gpio_detach_isr(gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif
