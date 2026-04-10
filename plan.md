# MyrtIO Framework — Architecture & Migration Plan

Pure C framework for ESP32 firmware development.
Built on top of ESP-IDF, replaces Arduino-esp32 core entirely.

---

## Design Principles

1. **Pure C11** — no C++ classes, no templates, no overloading
2. **Semantic naming** — module prefixes describe *what it does*, not what peripheral it maps to
3. **No ESP-IDF name collisions** — every public symbol uses a name that doesn't exist in ESP-IDF
4. **Handle-based resources** — peripherals are opened/closed via opaque handles
5. **Consistent patterns** — every module follows `open/close`, `read/write`, `attach/detach`
6. **Minimal magic** — no weak aliases, no hidden auto-init; explicit is better than implicit

---

## Naming Strategy

ESP-IDF already occupies these C namespaces globally:
`gpio_*`, `adc_*`/`adc1_*`/`adc2_*`, `dac_*`, `ledc_*`, `uart_*`, `spi_*`/`spi_bus_*`/`spi_device_*`,
`i2c_*`/`i2c_driver_*`/`i2c_master_*`, `timer_*`/`gptimer_*`, `rmt_*`, `touch_pad_*`,
`sigmadelta_*`/`sdm_*`, `esp_*`, `ESP_LOG*`.

MyrtIO avoids every one of them by using **different root words**:

| Domain           | MyrtIO prefix | Avoids ESP-IDF   | Rationale                           |
|------------------|---------------|------------------|-------------------------------------|
| Digital I/O      | `pin_`        | `gpio_`          | Describes the user-level concept    |
| Analog input     | `analog_`     | `adc_*`          | What it does, not the peripheral    |
| Analog output    | `dac_`        | `dac_output_*`   | Short, no func-name overlap         |
| PWM              | `pwm_`        | `ledc_`          | Universal PWM term                  |
| Hardware timers  | `tmr_`        | `timer_*`, `gptimer_*` | Compact, zero collision       |
| UART / serial    | `serial_`     | `uart_*`         | High-level semantic name            |
| SPI bus          | `spi_`        | `spi_bus_*`, `spi_device_*` | No exact func overlap    |
| I2C bus          | `i2c_`        | `i2c_driver_*`, `i2c_master_*` | No exact func overlap |
| Capacitive touch | `touch_`      | `touch_pad_*`    | Shorter, no func overlap            |
| Pulse I/O (RMT)  | `pulse_`      | `rmt_*`          | Describes what RMT actually does    |
| Sigma-delta      | `sigdelta_`   | `sigmadelta_*`, `sdm_*` | Compact alternative         |
| Timing / delay   | `delay_*`, `uptime_*` | —         | Self-documenting verbs              |
| CPU / clocks     | `clock_`      | —                | Unambiguous                         |
| System info      | `sys_`        | `esp_*`          | Generic, clean                      |
| Logging          | `log_`        | `ESP_LOG*`       | Standard short name                 |
| RGB LED          | `led_`        | —                | Obvious                             |

### Naming Rules

- **Functions**: `module_verb[_object]()` — e.g. `pin_set_mode()`, `serial_read()`, `tmr_open()`
- **Types**: `module_noun_t` — e.g. `pin_mode_t`, `serial_t`, `err_t`
- **Enum values**: `MODULE_CONSTANT` — e.g. `PIN_INPUT`, `LOG_ERROR`
- **Handles**: opaque `struct module_s` exposed as `module_t *`

---

## Common Types (`core.h`)

Shared across all modules. This is the only "framework-level" header.

```c
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ---- Version ----
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
typedef uint32_t ms_t;   // milliseconds
typedef uint64_t us_t;   // microseconds

// ---- Generic callback ----
typedef void (*callback_t)(void *arg);
```

**Error handling contract**:
- Functions that cannot fail → return `void` or value directly
- Functions that may fail simply → return `bool`
- Functions with multiple failure modes → return `err_t`
- Functions creating a resource → return handle pointer (`NULL` on failure)

---

## Entry Point

Arduino uses `setup()` + `loop()`. MyrtIO uses a single function:

```c
// User implements this. Called once from a FreeRTOS task.
void myrtio_main(void);
```

The framework's internal `app_main()` (ESP-IDF entry):
1. Initializes chip (CPU freq, NVS, PSRAM)
2. Creates a FreeRTOS task
3. Calls `myrtio_main()`
4. When `myrtio_main()` returns, the task idles (or optionally deletes itself)

No hidden `loop()`. The user writes their own loop or uses RTOS tasks directly.

Minimal user program:

```c
#include <myrtio/core.h>
#include <myrtio/pin.h>
#include <myrtio/clock.h>

void myrtio_main(void) {
    pin_set_mode(2, PIN_OUTPUT);
    for (;;) {
        pin_write(2, PIN_HIGH);
        delay_ms(500);
        pin_write(2, PIN_LOW);
        delay_ms(500);
    }
}
```

---

## Module Reference

### `myrtio/pin.h` — Digital I/O

Replaces: `esp32-hal-gpio.h`, `pinMode`, `digitalWrite`, `digitalRead`, `attachInterrupt`

```c
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
```

**Implementation**: wraps ESP-IDF `gpio_config()`, `gpio_set_level()`, `gpio_get_level()`, `gpio_isr_handler_add()`.

**Migration from Arduino core**:

| Arduino                   | MyrtIO                              |
|---------------------------|-------------------------------------|
| `pinMode(pin, OUTPUT)`    | `pin_set_mode(pin, PIN_OUTPUT)`     |
| `digitalWrite(pin, HIGH)` | `pin_write(pin, PIN_HIGH)`          |
| `digitalRead(pin)`        | `pin_read(pin)`                     |
| `attachInterrupt(pin, fn, RISING)` | `pin_attach_isr(pin, fn, arg, PIN_ISR_RISING)` |
| `detachInterrupt(pin)`    | `pin_detach_isr(pin)`               |

**Key difference from current `myrtio/gpio.h`**: no `gpio_` prefix (avoids ESP-IDF conflict),
interrupt support included, functions are not inline in the header.

---

### `myrtio/analog.h` — ADC & DAC

Replaces: `esp32-hal-adc.h`, `esp32-hal-dac.h`, `analogRead`, `dacWrite`

```c
#pragma once
#include "core.h"

// ---- ADC attenuation ----
typedef enum {
    ANALOG_ATTEN_0DB   = 0,
    ANALOG_ATTEN_2_5DB = 1,
    ANALOG_ATTEN_6DB   = 2,
    ANALOG_ATTEN_11DB  = 3,
} analog_atten_t;

// ---- ADC ----
uint16_t analog_read(pin_t pin);
uint32_t analog_read_mv(pin_t pin);
void     analog_set_resolution(uint8_t bits);
void     analog_set_atten(analog_atten_t atten);
void     analog_set_pin_atten(pin_t pin, analog_atten_t atten);

// ---- DAC ----
void dac_write(pin_t pin, uint8_t value);
void dac_disable(pin_t pin);
```

**Implementation**: wraps ESP-IDF `adc_oneshot_*` (v5) or `adc1_get_raw()` (v4.4),
`dac_output_voltage()`.

**Migration**:

| Arduino                 | MyrtIO                         |
|-------------------------|--------------------------------|
| `analogRead(pin)`       | `analog_read(pin)`             |
| `analogReadMilliVolts(pin)` | `analog_read_mv(pin)`      |
| `analogReadResolution(bits)` | `analog_set_resolution(bits)` |
| `analogSetAttenuation(atten)` | `analog_set_atten(atten)` |
| `dacWrite(pin, value)`  | `dac_write(pin, value)`        |
| `dacDisable(pin)`       | `dac_disable(pin)`             |

---

### `myrtio/pwm.h` — PWM Output

Replaces: `esp32-hal-ledc.h`, `ledcSetup`, `ledcWrite`, `ledcAttachPin`, `analogWrite`

```c
#pragma once
#include "core.h"

// ---- Musical notes (for tone generation) ----
typedef enum {
    NOTE_C, NOTE_Cs, NOTE_D, NOTE_Eb, NOTE_E, NOTE_F,
    NOTE_Fs, NOTE_G, NOTE_Gs, NOTE_A, NOTE_Bb, NOTE_B,
} note_t;

// ---- Channel-based API (full control) ----
uint32_t pwm_setup(uint8_t channel, uint32_t freq, uint8_t resolution);
void     pwm_write(uint8_t channel, uint32_t duty);
uint32_t pwm_read(uint8_t channel);
uint32_t pwm_get_freq(uint8_t channel);
uint32_t pwm_set_freq(uint8_t channel, uint32_t freq, uint8_t resolution);

void pwm_attach(uint8_t channel, pin_t pin);
void pwm_detach(pin_t pin);

uint32_t pwm_tone(uint8_t channel, uint32_t freq);
uint32_t pwm_note(uint8_t channel, note_t note, uint8_t octave);

// ---- Simple API (auto-managed channel) ----
void pwm_write_pin(pin_t pin, uint8_t value);
void pwm_set_pin_freq(uint32_t freq);
void pwm_set_pin_resolution(uint8_t bits);
```

`pwm_write_pin()` is the simple equivalent of Arduino's `analogWrite()` —
auto-allocates a LEDC channel for the pin.

**Migration**:

| Arduino                          | MyrtIO                                   |
|----------------------------------|------------------------------------------|
| `ledcSetup(ch, freq, bits)`      | `pwm_setup(ch, freq, bits)`              |
| `ledcAttachPin(pin, ch)`         | `pwm_attach(ch, pin)`                    |
| `ledcWrite(ch, duty)`            | `pwm_write(ch, duty)`                    |
| `ledcDetachPin(pin)`             | `pwm_detach(pin)`                        |
| `ledcWriteTone(ch, freq)`        | `pwm_tone(ch, freq)`                     |
| `ledcWriteNote(ch, note, oct)`   | `pwm_note(ch, note, oct)`                |
| `analogWrite(pin, value)`        | `pwm_write_pin(pin, value)`              |

---

### `myrtio/tmr.h` — Hardware Timers

Replaces: `esp32-hal-timer.h`, `timerBegin`, `timerAttachInterrupt`, etc.

Uses `tmr_` prefix to avoid collision with ESP-IDF's `timer_init`, `timer_start`, etc.

```c
#pragma once
#include "core.h"

typedef struct tmr_s tmr_t;
typedef void (*tmr_isr_t)(void *arg);

// ---- Lifecycle ----
tmr_t *tmr_open(uint8_t num, uint16_t divider, bool count_up);
void   tmr_close(tmr_t *t);

// ---- Control ----
void tmr_start(tmr_t *t);
void tmr_stop(tmr_t *t);
void tmr_restart(tmr_t *t);

// ---- Counter ----
void     tmr_write(tmr_t *t, uint64_t value);
uint64_t tmr_read(tmr_t *t);
uint64_t tmr_read_us(tmr_t *t);
double   tmr_read_sec(tmr_t *t);

// ---- Configuration ----
void     tmr_set_divider(tmr_t *t, uint16_t divider);
uint16_t tmr_get_divider(tmr_t *t);
void     tmr_set_count_up(tmr_t *t, bool up);
void     tmr_set_auto_reload(tmr_t *t, bool enable);

// ---- Alarm ----
void     tmr_set_alarm(tmr_t *t, uint64_t value);
void     tmr_enable_alarm(tmr_t *t);
void     tmr_disable_alarm(tmr_t *t);
uint64_t tmr_read_alarm(tmr_t *t);

// ---- Interrupt ----
void tmr_attach_isr(tmr_t *t, tmr_isr_t handler, void *arg);
void tmr_detach_isr(tmr_t *t);
```

**Migration**:

| Arduino                          | MyrtIO                           |
|----------------------------------|----------------------------------|
| `timerBegin(num, div, countUp)`  | `tmr_open(num, div, count_up)`   |
| `timerEnd(t)`                    | `tmr_close(t)`                   |
| `timerStart(t)`                  | `tmr_start(t)`                   |
| `timerAlarmWrite(t, val, reload)`| `tmr_set_alarm(t, val)` + `tmr_set_auto_reload(t, reload)` |
| `timerAlarmEnable(t)`            | `tmr_enable_alarm(t)`            |
| `timerAttachInterrupt(t, fn, edge)` | `tmr_attach_isr(t, fn, arg)` |
| `timerRead(t)`                   | `tmr_read(t)`                    |

---

### `myrtio/serial.h` — UART

Replaces: `esp32-hal-uart.h`, `HardwareSerial` class

```c
#pragma once
#include "core.h"

typedef struct serial_s serial_t;

// ---- Lifecycle ----
serial_t *serial_open(uint8_t port, uint32_t baud, uint32_t config,
                      int8_t rx_pin, int8_t tx_pin);
void      serial_close(serial_t *s);

// ---- Read ----
size_t  serial_available(serial_t *s);
size_t  serial_read(serial_t *s, uint8_t *buf, size_t len, ms_t timeout);
uint8_t serial_read_byte(serial_t *s);
uint8_t serial_peek(serial_t *s);

// ---- Write ----
size_t serial_write(serial_t *s, const uint8_t *data, size_t len);
void   serial_write_byte(serial_t *s, uint8_t byte);
void   serial_flush(serial_t *s);

// ---- Configuration ----
void     serial_set_baud(serial_t *s, uint32_t baud);
uint32_t serial_get_baud(serial_t *s);
bool     serial_set_pins(serial_t *s, int8_t rx, int8_t tx, int8_t cts, int8_t rts);
void     serial_set_rx_invert(serial_t *s, bool invert);

// ---- Debug output ----
void serial_set_debug(serial_t *s);
```

**Migration**:

| Arduino                         | MyrtIO                                          |
|---------------------------------|-------------------------------------------------|
| `Serial.begin(baud)`           | `serial_t *s = serial_open(0, baud, cfg, rx, tx)` |
| `Serial.available()`           | `serial_available(s)`                            |
| `Serial.read()`                | `serial_read_byte(s)`                            |
| `Serial.write(buf, len)`       | `serial_write(s, buf, len)`                      |
| `Serial.flush()`               | `serial_flush(s)`                                |
| `Serial.end()`                 | `serial_close(s)`                                |

---

### `myrtio/spi.h` — SPI Bus

Replaces: `esp32-hal-spi.h`

Function names (`spi_open`, `spi_close`, `spi_transfer`, etc.) do not collide with
ESP-IDF (`spi_bus_initialize`, `spi_device_transmit`, etc.).

```c
#pragma once
#include "core.h"

typedef struct spi_s spi_t;

#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

#define SPI_MSBFIRST 1
#define SPI_LSBFIRST 0

// ---- Lifecycle ----
spi_t *spi_open(uint8_t bus, uint32_t clock_hz, uint8_t mode, uint8_t bit_order);
void   spi_close(spi_t *s);

// ---- Pin assignment ----
void spi_attach_pins(spi_t *s, int8_t sck, int8_t miso, int8_t mosi);
void spi_attach_ss(spi_t *s, int8_t ss);
void spi_detach_ss(spi_t *s);

// ---- Configuration ----
void spi_set_clock(spi_t *s, uint32_t clock_hz);
void spi_set_mode(spi_t *s, uint8_t mode);
void spi_set_bit_order(spi_t *s, uint8_t order);

// ---- Transfer ----
uint8_t  spi_transfer_byte(spi_t *s, uint8_t data);
uint16_t spi_transfer_word(spi_t *s, uint16_t data);
uint32_t spi_transfer_long(spi_t *s, uint32_t data);
void     spi_transfer(spi_t *s, const uint8_t *tx, uint8_t *rx, size_t len);

// ---- Write-only (faster, ignores MISO) ----
void spi_write_byte(spi_t *s, uint8_t data);
void spi_write_word(spi_t *s, uint16_t data);
void spi_write(spi_t *s, const uint8_t *data, size_t len);

// ---- Transaction (bus locking) ----
void spi_begin_transaction(spi_t *s);
void spi_end_transaction(spi_t *s);
```

---

### `myrtio/i2c.h` — I2C Bus

Replaces: `esp32-hal-i2c.h`, `esp32-hal-i2c-slave.h`

Function names (`i2c_open`, `i2c_read`, `i2c_write`) do not collide with
ESP-IDF (`i2c_driver_install`, `i2c_master_write_to_device`, etc.).

```c
#pragma once
#include "core.h"

// ---- Master ----
err_t i2c_open(uint8_t port, pin_t sda, pin_t scl, uint32_t freq_hz);
err_t i2c_close(uint8_t port);
bool  i2c_is_open(uint8_t port);

err_t i2c_write(uint8_t port, uint16_t addr,
                const uint8_t *data, size_t len, ms_t timeout);
err_t i2c_read(uint8_t port, uint16_t addr,
               uint8_t *data, size_t len, ms_t timeout);
err_t i2c_write_read(uint8_t port, uint16_t addr,
                     const uint8_t *tx, size_t tx_len,
                     uint8_t *rx, size_t rx_len, ms_t timeout);

err_t i2c_set_clock(uint8_t port, uint32_t freq_hz);
err_t i2c_get_clock(uint8_t port, uint32_t *freq_hz);

// ---- Slave ----
typedef void (*i2c_slave_request_cb_t)(uint8_t port, void *arg);
typedef void (*i2c_slave_receive_cb_t)(uint8_t port, uint8_t *data,
                                       size_t len, bool stop, void *arg);

err_t  i2c_slave_open(uint8_t port, pin_t sda, pin_t scl,
                      uint16_t addr, uint32_t freq_hz,
                      size_t rx_buf, size_t tx_buf);
err_t  i2c_slave_close(uint8_t port);
size_t i2c_slave_write(uint8_t port, const uint8_t *data, size_t len, ms_t timeout);
err_t  i2c_slave_on_request(uint8_t port, i2c_slave_request_cb_t cb, void *arg);
err_t  i2c_slave_on_receive(uint8_t port, i2c_slave_receive_cb_t cb, void *arg);
```

---

### `myrtio/touch.h` — Capacitive Touch

Replaces: `esp32-hal-touch.h`

```c
#pragma once
#include "core.h"

typedef uint32_t touch_value_t;
typedef void (*touch_isr_t)(void *arg);

touch_value_t touch_read(pin_t pin);
void          touch_set_cycles(uint16_t measure, uint16_t sleep);

void touch_attach_isr(pin_t pin, touch_isr_t handler, void *arg,
                      touch_value_t threshold);
void touch_detach_isr(pin_t pin);
void touch_sleep_wakeup(pin_t pin, touch_value_t threshold);
```

---

### `myrtio/pulse.h` — Pulse I/O (RMT)

Replaces: `esp32-hal-rmt.h`

The name `pulse` describes the actual use case (IR signals, NeoPixel, distance sensors)
rather than the peripheral name.

```c
#pragma once
#include "core.h"

typedef struct pulse_s pulse_t;

typedef union {
    struct {
        uint32_t duration0 : 15;
        uint32_t level0    : 1;
        uint32_t duration1 : 15;
        uint32_t level1    : 1;
    };
    uint32_t raw;
} pulse_data_t;

typedef void (*pulse_rx_cb_t)(uint32_t *data, size_t len, void *arg);

// ---- Lifecycle ----
pulse_t *pulse_open_tx(pin_t pin, uint8_t mem_blocks);
pulse_t *pulse_open_rx(pin_t pin, uint8_t mem_blocks);
void     pulse_close(pulse_t *p);

// ---- Configuration ----
float pulse_set_tick(pulse_t *p, float tick_ns);
bool  pulse_set_rx_threshold(pulse_t *p, uint16_t value);
bool  pulse_set_filter(pulse_t *p, uint8_t ticks);
bool  pulse_set_carrier(pulse_t *p, bool enable, bool level,
                        uint32_t freq, float duty);

// ---- Transmit ----
bool pulse_write(pulse_t *p, const pulse_data_t *data, size_t len);
bool pulse_write_blocking(pulse_t *p, const pulse_data_t *data, size_t len);
bool pulse_loop(pulse_t *p, const pulse_data_t *data, size_t len);

// ---- Receive ----
bool pulse_read(pulse_t *p, pulse_rx_cb_t cb, void *arg);
bool pulse_read_async(pulse_t *p, pulse_data_t *data, size_t *len);
```

---

### `myrtio/sigdelta.h` — Sigma-Delta Modulation

Replaces: `esp32-hal-sigmadelta.h`

```c
#pragma once
#include "core.h"

uint32_t sigdelta_setup(pin_t pin, uint8_t channel, uint32_t freq);
void     sigdelta_write(uint8_t channel, uint8_t duty);
uint8_t  sigdelta_read(uint8_t channel);
void     sigdelta_detach(pin_t pin);
```

---

### `myrtio/clock.h` — Timing & CPU Frequency

Replaces: `millis`, `micros`, `delay`, `delayMicroseconds`,
`esp32-hal-cpu.h`, `setCpuFrequencyMhz`, etc.

```c
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
```

**Migration**:

| Arduino                       | MyrtIO                        |
|-------------------------------|-------------------------------|
| `delay(ms)`                   | `delay_ms(ms)`                |
| `delayMicroseconds(us)`       | `delay_us(us)`                |
| `millis()`                    | `uptime_ms()`                 |
| `micros()`                    | `uptime_us()`                 |
| `yield()`                     | `yield()`                     |
| `setCpuFrequencyMhz(mhz)`    | `clock_set_cpu_mhz(mhz)`     |
| `getCpuFrequencyMhz()`       | `clock_get_cpu_mhz()`        |

---

### `myrtio/sys.h` — System Info, Power, Memory

Replaces: `Esp.h` (`EspClass` / `ESP` singleton), `esp32-hal-psram.h`

```c
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
bool     sys_psram_init(void);
bool     sys_psram_found(void);
uint32_t sys_psram_total(void);
uint32_t sys_psram_free(void);
void    *sys_psram_malloc(size_t size);
void    *sys_psram_calloc(size_t n, size_t size);
void    *sys_psram_realloc(void *ptr, size_t size);

// ---- Flash ----
uint32_t sys_flash_size(void);
uint32_t sys_flash_speed(void);

// ---- SDK / firmware ----
const char *sys_sdk_version(void);
uint32_t    sys_sketch_size(void);
uint32_t    sys_sketch_free_space(void);

// ---- WDT ----
void sys_wdt_enable(void);
void sys_wdt_disable(void);
void sys_wdt_feed(void);
```

**Migration**:

| Arduino                   | MyrtIO                    |
|---------------------------|---------------------------|
| `ESP.restart()`           | `sys_restart()`           |
| `ESP.deepSleep(us)`       | `sys_deep_sleep(us)`      |
| `ESP.getChipModel()`     | `sys_chip_model()`        |
| `ESP.getFreeHeap()`      | `sys_heap_free()`         |
| `ESP.getPsramSize()`     | `sys_psram_total()`       |
| `ESP.getFlashChipSize()` | `sys_flash_size()`        |
| `temperatureRead()`      | `sys_temperature()`       |
| `psramFound()`           | `sys_psram_found()`       |
| `ps_malloc(size)`        | `sys_psram_malloc(size)`  |

---

### `myrtio/log.h` — Logging

Replaces: `esp32-hal-log.h`

```c
#pragma once

#include <stdint.h>

enum {
    LOG_NONE    = 0,
    LOG_ERROR   = 1,
    LOG_WARN    = 2,
    LOG_INFO    = 3,
    LOG_DEBUG   = 4,
    LOG_VERBOSE = 5,
};

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

int  log_printf(const char *fmt, ...);
void log_print_buf(const uint8_t *buf, size_t len);

#define log_e(tag, fmt, ...) /* error   — red */
#define log_w(tag, fmt, ...) /* warning — yellow */
#define log_i(tag, fmt, ...) /* info    — green */
#define log_d(tag, fmt, ...) /* debug   — no color */
#define log_v(tag, fmt, ...) /* verbose — no color */
```

Macros expand to `log_printf(...)` when the level is enabled, to nothing otherwise.
Uses `__FILE__` and `__LINE__` automatically. Supports compile-time filtering
via `LOG_LEVEL` and per-module override via `LOG_LOCAL_LEVEL`.

---

### `myrtio/led.h` — Onboard RGB LED

Replaces: `esp32-hal-rgb-led.h`

```c
#pragma once
#include "core.h"

void led_write_rgb(pin_t pin, uint8_t r, uint8_t g, uint8_t b);
```

Implementation uses the pulse (RMT) module internally.

---

## What Gets Dropped

These Arduino-core features are **not migrated** — they are either C++ specific,
redundant, or belong in libraries:

| Arduino Feature              | Reason to Drop                                      |
|------------------------------|-----------------------------------------------------|
| `String` class               | C++ only. Use `char *` + standard C string functions |
| `Stream`, `Print` classes    | C++ only. Replaced by `serial_write`/`log_printf`   |
| `HardwareSerial` class       | C++ only. Replaced by `serial_*` functions          |
| `IPAddress`, `Client`, `Server`, `Udp` | C++ networking classes → library territory |
| `Esp` class (C++ singleton)  | Replaced by `sys_*` functions                       |
| `FirmwareMSC`, `USBCDC`, `USB`, `USBMSC`, `HWCDC` | C++ USB classes → library territory |
| `FunctionalInterrupt`        | C++ `std::function` wrappers — use function pointers |
| `Tone` / `noTone`            | Use `pwm_tone()` directly                           |
| `shiftIn` / `shiftOut`       | Trivial bitbang — user utility, not core            |
| `pulseIn` / `pulseInLong`    | Niche — can be a utility or use `pulse_*` module    |
| `WCharacter.h`               | Use standard `<ctype.h>`                            |
| `binary.h`                   | Use C standard `0b...` literals                     |
| `pgmspace.h`                 | ESP32 has unified address space — unnecessary       |
| `esp8266-compat.h`           | Irrelevant                                          |
| `base64` / `MD5Builder`      | Library territory                                   |
| `cbuf` (circular buffer)     | Library territory                                   |
| `io_pin_remap.h`             | Macro-based pin remapping. Replace with simple mapping table in board variant |
| Arduino math macros (`_min`, `_max`, `constrain`, etc.) | Keep a few essentials in `core.h`, drop rest |
| `random` / `randomSeed`      | Use ESP-IDF `esp_random()` or standard `rand()`     |
| `getLocalTime` / `configTime` | Library territory (NTP/time)                       |

---

## Utilities to Keep in `core.h`

Small set of useful macros that don't exist in standard C:

```c
#define MIN(a, b)          ((a) < (b) ? (a) : (b))
#define MAX(a, b)          ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)   (MIN(MAX(x, lo), hi))
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define BIT(n)              (1UL << (n))
#define BIT_SET(x, n)       ((x) |= BIT(n))
#define BIT_CLEAR(x, n)     ((x) &= ~BIT(n))
#define BIT_READ(x, n)      (((x) >> (n)) & 1)
#define UNUSED(x)           ((void)(x))
```

---

## File Organization

Final directory layout inside `cores/esp32/myrtio/`:

```
myrtio/
├── core.h            ← common types, macros, entry point declaration
├── core.c            ← app_main() implementation, init sequence
├── pin.h / pin.c
├── analog.h / analog.c
├── pwm.h / pwm.c
├── tmr.h / tmr.c
├── serial.h / serial.c
├── spi.h / spi.c
├── i2c.h / i2c.c
├── touch.h / touch.c
├── pulse.h / pulse.c
├── sigdelta.h / sigdelta.c
├── clock.h / clock.c
├── sys.h / sys.c
├── log.h / log.c
└── led.h / led.c
```

The top-level `myrtio.h` remains as a convenience include that pulls everything:

```c
#pragma once
#include "myrtio/core.h"
#include "myrtio/pin.h"
#include "myrtio/analog.h"
#include "myrtio/pwm.h"
#include "myrtio/tmr.h"
#include "myrtio/serial.h"
#include "myrtio/spi.h"
#include "myrtio/i2c.h"
#include "myrtio/touch.h"
#include "myrtio/pulse.h"
#include "myrtio/sigdelta.h"
#include "myrtio/clock.h"
#include "myrtio/sys.h"
#include "myrtio/log.h"
#include "myrtio/led.h"
```

---

## Implementation Strategy

Each `.c` file wraps ESP-IDF drivers directly. **No Arduino HAL in between.**
The Arduino `esp32-hal-*.c` files serve as reference for how to call ESP-IDF,
but are not linked or used.

Example — `pin.c`:

```c
#include "pin.h"
#include "driver/gpio.h"         // ESP-IDF
#include "esp_intr_alloc.h"     // ESP-IDF

void pin_set_mode(pin_t pin, pin_mode_t mode) {
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode         = /* translate pin_mode_t → gpio_mode_t */,
        .pull_up_en   = (mode & PIN_PULLUP) ? 1 : 0,
        .pull_down_en = (mode & PIN_PULLDOWN) ? 1 : 0,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
}

void pin_write(pin_t pin, pin_level_t level) {
    gpio_set_level(pin, level);
}

pin_level_t pin_read(pin_t pin) {
    return gpio_get_level(pin);
}
```

The translation table between MyrtIO `pin_mode_t` and ESP-IDF `gpio_mode_t` lives
entirely inside `pin.c` — it never leaks into the public API.

---

## Build System

### CMakeLists.txt changes

Replace the Arduino source list with MyrtIO sources:

```cmake
set(MYRTIO_SRCS
    "cores/esp32/myrtio/core.c"
    "cores/esp32/myrtio/pin.c"
    "cores/esp32/myrtio/analog.c"
    "cores/esp32/myrtio/pwm.c"
    "cores/esp32/myrtio/tmr.c"
    "cores/esp32/myrtio/serial.c"
    "cores/esp32/myrtio/spi.c"
    "cores/esp32/myrtio/i2c.c"
    "cores/esp32/myrtio/touch.c"
    "cores/esp32/myrtio/pulse.c"
    "cores/esp32/myrtio/sigdelta.c"
    "cores/esp32/myrtio/clock.c"
    "cores/esp32/myrtio/sys.c"
    "cores/esp32/myrtio/log.c"
    "cores/esp32/myrtio/led.c"
)
```

Include path: `cores/esp32` (so `#include <myrtio/pin.h>` resolves).

The old Arduino `.cpp` files (`Arduino.h`, `WString.cpp`, `Print.cpp`, `Stream.cpp`,
`HardwareSerial.cpp`, etc.) are **removed from the build** entirely.

The old HAL `.c` files (`esp32-hal-gpio.c`, etc.) are also removed — MyrtIO reimplements
the same functionality directly against ESP-IDF.

---

## Migration Order

Recommended implementation sequence, from foundational to dependent:

| Phase | Module         | Depends On     | Effort |
|-------|----------------|----------------|--------|
| 1     | `core.h`       | —              | S      |
| 1     | `log.h/.c`     | `core`         | S      |
| 1     | `clock.h/.c`   | `core`         | S      |
| 2     | `pin.h/.c`     | `core`, `log`  | M      |
| 2     | `sys.h/.c`     | `core`, `log`  | M      |
| 3     | `analog.h/.c`  | `core`, `pin`  | M      |
| 3     | `pwm.h/.c`     | `core`, `pin`  | M      |
| 3     | `touch.h/.c`   | `core`, `pin`  | S      |
| 4     | `serial.h/.c`  | `core`, `pin`  | L      |
| 4     | `tmr.h/.c`     | `core`, `clock`| M      |
| 5     | `spi.h/.c`     | `core`, `pin`  | L      |
| 5     | `i2c.h/.c`     | `core`, `pin`  | L      |
| 6     | `pulse.h/.c`   | `core`, `pin`  | M      |
| 6     | `led.h/.c`     | `pulse`        | S      |
| 6     | `sigdelta.h/.c`| `core`, `pin`  | S      |

**S** = small (< 100 LOC), **M** = medium (100–300 LOC), **L** = large (300+ LOC)

---

## ESP-IDF Conflict Summary

Full list of names to **never use** as MyrtIO public symbols:

| Conflict Zone           | ESP-IDF Symbols                        | MyrtIO Alternative     |
|-------------------------|----------------------------------------|------------------------|
| `gpio_mode_t`           | `GPIO_MODE_INPUT`, `GPIO_MODE_OUTPUT`… | `pin_mode_t`, `PIN_INPUT`, `PIN_OUTPUT`… |
| `gpio_set_direction()`  | Exists                                 | `pin_set_mode()`       |
| `gpio_set_level()`      | Exists                                 | `pin_write()`          |
| `gpio_get_level()`      | Exists                                 | `pin_read()`           |
| `adc_atten_t`           | `ADC_ATTEN_DB_0`, etc.                 | `analog_atten_t`, `ANALOG_ATTEN_0DB` |
| `timer_init()`          | Exists (legacy driver)                 | `tmr_open()`           |
| `timer_start()`         | Exists (legacy driver)                 | `tmr_start()`          |
| `uart_read_bytes()`     | Exists                                 | `serial_read()`        |
| `uart_write_bytes()`    | Exists                                 | `serial_write()`       |
| `rmt_config()`          | Exists                                 | `pulse_open_tx/rx()`   |
| `i2c_driver_install()`  | Exists                                 | `i2c_open()`           |
| `spi_bus_initialize()`  | Exists                                 | `spi_open()`           |
| `touch_pad_read()`      | Exists                                 | `touch_read()`         |
| `sigmadelta_config()`   | Exists                                 | `sigdelta_setup()`     |

---

## GPIO Matrix & Pin Remapping

Arduino's `pinMatrixInAttach`/`pinMatrixOutAttach` and `io_pin_remap.h` macro
system are **not exposed publicly**. Instead:

- GPIO matrix routing is an **internal implementation detail** used by `spi.c`, `serial.c`,
  `i2c.c`, `pulse.c` when attaching pins.
- Board-specific pin mapping (logical → physical) is handled via a **variant header**
  (`pins_variant.h`) that defines `pin_t` constants:

```c
// variants/myboard/pins_variant.h
#define PIN_LED       2
#define PIN_SDA       21
#define PIN_SCL       22
#define PIN_SPI_MOSI  23
#define PIN_SPI_MISO  19
#define PIN_SPI_SCK   18
#define PIN_UART0_TX  1
#define PIN_UART0_RX  3
```

No macro-based transparent remapping. The user passes physical GPIO numbers
or board-defined constants. Explicit and predictable.
