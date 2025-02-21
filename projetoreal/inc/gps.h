#ifndef GPS_H
#define GPS_H

#include "hardware/uart.h"
#include <stddef.h>
#include <stdbool.h>

// Função para ler dados do módulo GPS via UART.
// Retorna true se algum dado foi lido, armazenando em 'buffer'.
bool gps_read(uart_inst_t *uart, char *buffer, size_t len);

#endif
