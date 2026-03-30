#include "pin.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

static bool _isr_service_installed = false;

static gpio_mode_t _to_gpio_mode(pin_mode_t mode) {
    if (mode == PIN_ANALOG) {
        return GPIO_MODE_DISABLE;
    }
    bool is_output = (mode & 0x02) != 0;
    bool is_od = (mode & PIN_OPEN_DRAIN) != 0;
    if (is_output) {
        return is_od ? GPIO_MODE_INPUT_OUTPUT_OD : GPIO_MODE_OUTPUT;
    }
    return GPIO_MODE_INPUT;
}

void pin_set_mode(pin_t pin, pin_mode_t mode) {
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode         = _to_gpio_mode(mode),
        .pull_up_en   = (mode & PIN_PULLUP)   ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE,
        .pull_down_en = (mode & PIN_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
}

void pin_write(pin_t pin, pin_level_t level) {
    gpio_set_level((gpio_num_t)pin, (uint32_t)level);
}

pin_level_t pin_read(pin_t pin) {
    return (pin_level_t)gpio_get_level((gpio_num_t)pin);
}

void pin_attach_isr(pin_t pin, pin_isr_t handler, void *arg, pin_isr_mode_t mode) {
    gpio_int_type_t intr_type;
    switch (mode) {
        case PIN_ISR_RISING:  intr_type = GPIO_INTR_POSEDGE;    break;
        case PIN_ISR_FALLING: intr_type = GPIO_INTR_NEGEDGE;    break;
        case PIN_ISR_CHANGE:  intr_type = GPIO_INTR_ANYEDGE;    break;
        case PIN_ISR_LOW:     intr_type = GPIO_INTR_LOW_LEVEL;  break;
        case PIN_ISR_HIGH:    intr_type = GPIO_INTR_HIGH_LEVEL; break;
        default:              intr_type = GPIO_INTR_DISABLE;    break;
    }
    gpio_set_intr_type((gpio_num_t)pin, intr_type);
    if (!_isr_service_installed) {
        gpio_install_isr_service(0);
        _isr_service_installed = true;
    }
    gpio_isr_handler_add((gpio_num_t)pin, handler, arg);
}

void pin_detach_isr(pin_t pin) {
    gpio_isr_handler_remove((gpio_num_t)pin);
}

bool pin_is_valid(pin_t pin) {
    return GPIO_IS_VALID_GPIO(pin);
}

bool pin_can_output(pin_t pin) {
    return GPIO_IS_VALID_OUTPUT_GPIO(pin);
}

// ESP32 ADC1 channel mapping (GPIO → ADC channel)
int8_t pin_to_adc_channel(pin_t pin) {
    switch (pin) {
        case 36: return 0;
        case 37: return 1;
        case 38: return 2;
        case 39: return 3;
        case 32: return 4;
        case 33: return 5;
        case 34: return 6;
        case 35: return 7;
        default: return -1;
    }
}

int8_t pin_to_dac_channel(pin_t pin) {
    switch (pin) {
        case 25: return 0;
        case 26: return 1;
        default: return -1;
    }
}

int8_t pin_to_touch_channel(pin_t pin) {
    switch (pin) {
        case 4:  return 0;
        case 0:  return 1;
        case 2:  return 2;
        case 15: return 3;
        case 13: return 4;
        case 12: return 5;
        case 14: return 6;
        case 27: return 7;
        case 33: return 8;
        case 32: return 9;
        default: return -1;
    }
}
