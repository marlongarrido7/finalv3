#include "gps.h"
#include "pico/stdlib.h"
#include <stdio.h>

bool gps_read(uart_inst_t *uart, char *buffer, size_t len) {
    size_t count = 0;
    while (uart_is_readable(uart) && count < len - 1) {
        char c = uart_getc(uart);
        buffer[count++] = c;
        if (c == '\n') break;
    }
    buffer[count] = '\0';
    return (count > 0);
}
