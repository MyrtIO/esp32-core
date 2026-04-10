#include "serial.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#define SERIAL_RX_BUF 256
#define SERIAL_TX_BUF 256

struct serial_s {
    uart_port_t port;
    bool has_peek;
    uint8_t peek_byte;
};

// Config encoding: parity[1:0] | data_bits[3:2] | stop_bits[5:4]
// These values map directly to uart_word_length_t / uart_parity_t / uart_stop_bits_t.
static uart_word_length_t _data_bits(uint32_t config) {
    return (uart_word_length_t)((config >> 2) & 0x3);
}

static uart_parity_t _parity(uint32_t config) {
    return (uart_parity_t)(config & 0x3);
}

static uart_stop_bits_t _stop_bits(uint32_t config) {
    return (uart_stop_bits_t)((config >> 4) & 0x3);
}

serial_t *serial_open(uint8_t port, uint32_t baud, uint32_t config,
                      int8_t rx_pin, int8_t tx_pin) {
    uart_config_t uart_cfg = {
        .baud_rate  = (int)baud,
        .data_bits  = _data_bits(config),
        .parity     = _parity(config),
        .stop_bits  = _stop_bits(config),
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    if (uart_driver_install(port, SERIAL_RX_BUF, SERIAL_TX_BUF,
                            0, NULL, 0) != ESP_OK) {
        return NULL;
    }
    if (uart_param_config(port, &uart_cfg) != ESP_OK) {
        uart_driver_delete(port);
        return NULL;
    }
    if (uart_set_pin(port, tx_pin, rx_pin,
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        uart_driver_delete(port);
        return NULL;
    }

    serial_t *s = (serial_t *)malloc(sizeof(serial_t));
    if (!s) {
        uart_driver_delete(port);
        return NULL;
    }
    s->port = (uart_port_t)port;
    s->has_peek = false;
    s->peek_byte = 0;
    return s;
}

void serial_close(serial_t *s) {
    if (!s) return;
    uart_driver_delete(s->port);
    free(s);
}

size_t serial_available(serial_t *s) {
    if (!s) return 0;
    size_t len = 0;
    uart_get_buffered_data_len(s->port, &len);
    return len + (s->has_peek ? 1 : 0);
}

size_t serial_read(serial_t *s, uint8_t *buf, size_t len, ms_t timeout) {
    if (!s || !buf || len == 0) return 0;
    size_t read = 0;
    if (s->has_peek && len > 0) {
        buf[0] = s->peek_byte;
        s->has_peek = false;
        buf++;
        len--;
        read++;
    }
    if (len > 0) {
        int n = uart_read_bytes(s->port, buf, (uint32_t)len,
                                pdMS_TO_TICKS(timeout));
        if (n > 0) read += (size_t)n;
    }
    return read;
}

uint8_t serial_read_byte(serial_t *s) {
    if (!s) return 0;
    if (s->has_peek) {
        s->has_peek = false;
        return s->peek_byte;
    }
    uint8_t byte = 0;
    uart_read_bytes(s->port, &byte, 1, portMAX_DELAY);
    return byte;
}

uint8_t serial_peek(serial_t *s) {
    if (!s) return 0;
    if (!s->has_peek) {
        int n = uart_read_bytes(s->port, &s->peek_byte, 1, 0);
        s->has_peek = (n == 1);
    }
    return s->peek_byte;
}

size_t serial_write(serial_t *s, const uint8_t *data, size_t len) {
    if (!s || !data || len == 0) return 0;
    int n = uart_write_bytes(s->port, (const void *)data, len);
    return (n < 0) ? 0 : (size_t)n;
}

void serial_write_byte(serial_t *s, uint8_t byte) {
    if (!s) return;
    uart_write_bytes(s->port, (const void *)&byte, 1);
}

void serial_flush(serial_t *s) {
    if (!s) return;
    uart_flush(s->port);
}

void serial_set_baud(serial_t *s, uint32_t baud) {
    if (!s) return;
    uart_set_baudrate(s->port, baud);
}

uint32_t serial_get_baud(serial_t *s) {
    if (!s) return 0;
    uint32_t baud = 0;
    uart_get_baudrate(s->port, &baud);
    return baud;
}

bool serial_set_pins(serial_t *s, int8_t rx, int8_t tx, int8_t cts, int8_t rts) {
    if (!s) return false;
    return uart_set_pin(s->port, tx, rx, rts, cts) == ESP_OK;
}

void serial_set_rx_invert(serial_t *s, bool invert) {
    if (!s) return;
    uart_set_line_inverse(s->port,
        invert ? UART_SIGNAL_RXD_INV : UART_SIGNAL_INV_DISABLE);
}

// Static serial handle used for debug output redirect
static serial_t *_debug_serial = NULL;

static int _serial_vprintf(const char *fmt, va_list args) {
    if (!_debug_serial) return 0;
    char buf[256];
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    if (n > 0) {
        uart_write_bytes(_debug_serial->port, buf,
                         (size_t)(n < (int)sizeof(buf) ? n : (int)sizeof(buf) - 1));
    }
    return n;
}

void serial_set_debug(serial_t *s) {
    _debug_serial = s;
    esp_log_set_vprintf(_serial_vprintf);
}
