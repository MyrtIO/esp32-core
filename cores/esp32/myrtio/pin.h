#pragma once
#include "core.h"

// ---- Pin modes ----
typedef enum {
    PIN_INPUT          = 0x01,
    PIN_OUTPUT         = 0x03,
    PIN_PULLUP         = 0x04,
    PIN_INPUT_PULLUP   = 0x05,
    PIN_PULLDOWN       = 0x08,
    PIN_INPUT_PULLDOWN = 0x09,
    PIN_OPEN_DRAIN     = 0x10,
    PIN_OUTPUT_OD      = 0x13,
    PIN_ANALOG         = 0xC0,
} pin_mode_t;

// ---- Pin logic level ----
typedef enum {
    PIN_LOW  = 0,
    PIN_HIGH = 1,
} pin_level_t;

// ---- Interrupt trigger ----
typedef enum {
    PIN_ISR_RISING  = 1,
    PIN_ISR_FALLING = 2,
    PIN_ISR_CHANGE  = 3,
    PIN_ISR_LOW     = 4,
    PIN_ISR_HIGH    = 5,
} pin_isr_mode_t;

typedef void (*pin_isr_t)(void *arg);

// ---- Functions ----
void        pin_set_mode(pin_t pin, pin_mode_t mode);
void        pin_write(pin_t pin, pin_level_t level);
pin_level_t pin_read(pin_t pin);

void pin_attach_isr(pin_t pin, pin_isr_t handler, void *arg, pin_isr_mode_t mode);
void pin_detach_isr(pin_t pin);

// ---- Pin queries ----
bool   pin_is_valid(pin_t pin);
bool   pin_can_output(pin_t pin);
int8_t pin_to_adc_channel(pin_t pin);
int8_t pin_to_touch_channel(pin_t pin);
int8_t pin_to_dac_channel(pin_t pin);
