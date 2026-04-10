#pragma once
#include "core.h"

typedef struct serial_s serial_t;

// Serial config encoding: parity[1:0] | data_bits[3:2] | stop_bits[5:4]
// Values map directly to ESP-IDF uart_parity_t / uart_word_length_t / uart_stop_bits_t.
#define SERIAL_5N1  0x10
#define SERIAL_6N1  0x14
#define SERIAL_7N1  0x18
#define SERIAL_8N1  0x1C
#define SERIAL_8N2  0x3C
#define SERIAL_8E1  0x1E
#define SERIAL_8O1  0x1F

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
