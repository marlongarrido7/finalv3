#include "ssd1306.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include <string.h>
#include "fonte.h"

#define SSD1306_CMD  0x00
#define SSD1306_DATA 0x40

// Define a margem interna para a borda
#define BORDER_MARGIN 2

static void ssd1306_send_command(ssd1306_t *dev, uint8_t cmd) {
    uint8_t data[2] = { SSD1306_CMD, cmd };
    i2c_write_blocking(dev->i2c, dev->address, data, 2, false);
}

void ssd1306_init(ssd1306_t *dev, i2c_inst_t *i2c, uint8_t address, uint8_t width, uint8_t height) {
    dev->i2c = i2c;
    dev->address = address;
    dev->width = width;
    dev->height = height;
    memset(dev->buffer, 0, sizeof(dev->buffer));

    ssd1306_send_command(dev, 0xAE);       // Display off
    ssd1306_send_command(dev, 0xD5);       // Set display clock divide ratio/oscillator frequency
    ssd1306_send_command(dev, 0x80);
    ssd1306_send_command(dev, 0xA8);       // Set multiplex ratio
    ssd1306_send_command(dev, height - 1);
    ssd1306_send_command(dev, 0xD3);       // Set display offset
    ssd1306_send_command(dev, 0x00);
    ssd1306_send_command(dev, 0x40);       // Set start line address
    ssd1306_send_command(dev, 0x8D);       // Charge pump setting
    ssd1306_send_command(dev, 0x14);
    ssd1306_send_command(dev, 0xA1);       // Set segment re-map
    ssd1306_send_command(dev, 0xC8);       // Set COM output scan direction
    ssd1306_send_command(dev, 0xDA);       // Set COM pins hardware configuration
    ssd1306_send_command(dev, 0x12);
    ssd1306_send_command(dev, 0x81);       // Set contrast control
    ssd1306_send_command(dev, 0xCF);
    ssd1306_send_command(dev, 0xD9);       // Set pre-charge period
    ssd1306_send_command(dev, 0xF1);
    ssd1306_send_command(dev, 0xDB);       // Set VCOMH deselect level
    ssd1306_send_command(dev, 0x40);
    ssd1306_send_command(dev, 0xA4);       // Entire display on from RAM
    ssd1306_send_command(dev, 0xA6);       // Normal display
    ssd1306_send_command(dev, 0xAF);       // Display on
}

void ssd1306_show(ssd1306_t *dev) {
    ssd1306_send_command(dev, 0x21);  // Set column address
    ssd1306_send_command(dev, 0);
    ssd1306_send_command(dev, dev->width - 1);
    ssd1306_send_command(dev, 0x22);  // Set page address
    ssd1306_send_command(dev, 0);
    ssd1306_send_command(dev, (dev->height / 8) - 1);
    
    uint8_t data[1 + sizeof(dev->buffer)];
    data[0] = SSD1306_DATA;
    memcpy(&data[1], dev->buffer, sizeof(dev->buffer));
    i2c_write_blocking(dev->i2c, dev->address, data, sizeof(data), false);
}

void ssd1306_clear(ssd1306_t *dev) {
    memset(dev->buffer, 0, sizeof(dev->buffer));
    ssd1306_show(dev);
}

void ssd1306_draw_pixel(ssd1306_t *dev, int x, int y, uint8_t color) {
    if (x < 0 || x >= dev->width || y < 0 || y >= dev->height) return;
    
    int index = x + (y / 8) * dev->width;
    uint8_t mask = 1 << (y % 8);
    
    if (color)
        dev->buffer[index] |= mask;
    else
        dev->buffer[index] &= ~mask;
}

static void ssd1306_draw_char(ssd1306_t *dev, int x, int y, char c) {
    const uint8_t *font = NULL;
    if (c >= 'A' && c <= 'Z')
        font = font_uppercase[c - 'A'];
    else if (c >= 'a' && c <= 'z')
        font = font_lowercase[c - 'a'];
    else if (c >= '0' && c <= '9')
        font = font_numbers[c - '0'];
    else
        return; // Ignora caractere não suportado
    
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            uint8_t pixel = font[i] & (1 << j);
            ssd1306_draw_pixel(dev, x + i, y + j, pixel ? 1 : 0);
        }
    }
}

void ssd1306_draw_string(ssd1306_t *dev, int x, int y, const char *str) {
    while (*str) {
        ssd1306_draw_char(dev, x, y, *str);
        x += 6;  // 5 pixels de largura + 1 pixel de espaço
        str++;
    }
    ssd1306_show(dev);
}

/**
 * @brief Desenha uma borda dentro do display, centralizada com uma margem.
 *
 * A borda é desenhada dentro da área definida por BORDER_MARGIN.
 * Exemplo: para um display de 128x64 e BORDER_MARGIN = 2, a borda será desenhada
 * de (2,2) até (125,61), deixando uma margem interna para os textos.
 */
void ssd1306_draw_border(ssd1306_t *dev, int thickness) {
    int w = dev->width;
    int h = dev->height;
    int margin = BORDER_MARGIN;
    int x_start = margin;
    int y_start = margin;
    int x_end = w - margin;   // Limite exclusivo
    int y_end = h - margin;   // Limite exclusivo

    // Borda superior
    for (int y = y_start; y < y_start + thickness; y++) {
        for (int x = x_start; x < x_end; x++) {
            ssd1306_draw_pixel(dev, x, y, 1);
        }
    }
    // Borda inferior
    for (int y = y_end - thickness; y < y_end; y++) {
        for (int x = x_start; x < x_end; x++) {
            ssd1306_draw_pixel(dev, x, y, 1);
        }
    }
    // Borda esquerda
    for (int x = x_start; x < x_start + thickness; x++) {
        for (int y = y_start; y < y_end; y++) {
            ssd1306_draw_pixel(dev, x, y, 1);
        }
    }
    // Borda direita
    for (int x = x_end - thickness; x < x_end; x++) {
        for (int y = y_start; y < y_end; y++) {
            ssd1306_draw_pixel(dev, x, y, 1);
        }
    }
    // A atualização do display (ssd1306_show) deve ser feita pelo chamador.
}
