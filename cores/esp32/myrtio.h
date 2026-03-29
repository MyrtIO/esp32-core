#ifndef MYRTIO_H
#define MYRTIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MYRTIO_PIN_MODE_INPUT = 0x01u,
    MYRTIO_PIN_MODE_OUTPUT = 0x03u,
    MYRTIO_DIGITAL_LOW = 0x00u,
    MYRTIO_DIGITAL_HIGH = 0x01u
};

void myrtio_main(void);
void myrtio_pin_mode(uint8_t pin, uint8_t mode);
void myrtio_digital_write(uint8_t pin, uint8_t value);
void myrtio_delay_ms(uint32_t ms);
uint8_t myrtio_builtin_led(void);

#ifdef __cplusplus
}
#endif

#endif
