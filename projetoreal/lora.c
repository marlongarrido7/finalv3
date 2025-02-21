#include "lora.h"
#include "hardware/uart.h"

void lora_send(uart_inst_t *uart, const char *message) {
    uart_puts(uart, message);
    uart_puts(uart, "\n");
}
