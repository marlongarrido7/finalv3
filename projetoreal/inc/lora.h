#ifndef LORA_H
#define LORA_H

#include "hardware/uart.h"

// Função para enviar mensagem via módulo LoRa
void lora_send(uart_inst_t *uart, const char *message);

#endif
