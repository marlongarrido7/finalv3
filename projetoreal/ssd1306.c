#include "ssd1306.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "fonte.h"
#include <string.h>

// Função auxiliar para enviar comandos
static void ssd1306_write_command(ssd1306_t *dev, uint8_t command) {
    uint8_t data[2] = {0x00, command};
    i2c_write_blocking(dev->i2c, dev->addr, data, 2, false);
}

void ssd1306_init(ssd1306_t *dev, i2c_inst_t *i2c, uint8_t addr, uint8_t width, uint8_t height) {
    dev->i2c = i2c;
    dev->addr = addr;
    dev->width = width;
    dev->height = height;
    memset(dev->buffer, 0, sizeof(dev->buffer));
    
    // Sequência de inicialização simplificada
    ssd1306_write_command(dev, 0xAE); // Display off
    ssd1306_write_command(dev, 0xD5); // Clock divide ratio/oscillator frequency
    ssd1306_write_command(dev, 0x80);
    ssd1306_write_command(dev, 0xA8); // Multiplex ratio
    ssd1306_write_command(dev, height - 1);
    ssd1306_write_command(dev, 0xD3); // Display offset
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, 0x40); // Start line address
    ssd1306_write_command(dev, 0x8D); // Charge pump
    ssd1306_write_command(dev, 0x14);
    ssd1306_write_command(dev, 0x20); // Memory addressing mode
    ssd1306_write_command(dev, 0x00);
    ssd1306_write_command(dev, 0xA1); // Segment re-map
    ssd1306_write_command(dev, 0xC8); // COM output scan direction
    ssd1306_write_command(dev, 0xDA); // COM pins hardware configuration
    ssd1306_write_command(dev, 0x12);
    ssd1306_write_command(dev, 0x81); // Contrast control
    ssd1306_write_command(dev, 0xCF);
    ssd1306_write_command(dev, 0xD9); // Pre-charge period
    ssd1306_write_command(dev, 0xF1);
    ssd1306_write_command(dev, 0xDB); // VCOMH deselect level
    ssd1306_write_command(dev, 0x40);
    ssd1306_write_command(dev, 0xA4); // Entire display on
    ssd1306_write_command(dev, 0xA6); // Normal display
    ssd1306_write_command(dev, 0xAF); // Display on
}

void ssd1306_clear(ssd1306_t *dev) {
    memset(dev->buffer, 0, sizeof(dev->buffer));
}

void ssd1306_show(ssd1306_t *dev) {
    ssd1306_write_command(dev, 0x21); // Set column address
    ssd1306_write_command(dev, 0);
    ssd1306_write_command(dev, dev->width - 1);
    
    ssd1306_write_command(dev, 0x22); // Set page address
    ssd1306_write_command(dev, 0);
    ssd1306_write_command(dev, (dev->height / 8) - 1);
    
    uint8_t data[17];
    data[0] = 0x40; // Modo de dados
    for (int i = 0; i < (dev->width * dev->height / 8); ) {
        int chunk = ((dev->width * dev->height) / 8) - i;
        if (chunk > 16) chunk = 16;
        memcpy(&data[1], &dev->buffer[i], chunk);
        i2c_write_blocking(dev->i2c, dev->addr, data, chunk + 1, false);
        i += chunk;
    }
}

void ssd1306_draw_string(ssd1306_t *dev, uint8_t x, uint8_t y, const char *str) {
    // Implementação simplificada: neste exemplo, não desenha o texto real.
    // Em uma aplicação real, a função copiará os bits do caractere do array de fonte (fonte.h)
    // para o buffer de display considerando a posição (x,y).
    // Aqui, apenas um stub para exemplificar.
    (void)dev; (void)x; (void)y; (void)str;
}

void ssd1306_draw_border(ssd1306_t *dev, int thickness) {
    // Desenha uma borda simples no buffer
    for (int t = 0; t < thickness; t++) {
        // Linhas superior e inferior
        for (int x = 0; x < dev->width; x++) {
            int page_top = (0 + t) / 8;
            int page_bottom = (dev->height - 1 - t) / 8;
            int bit_top = (0 + t) % 8;
            int bit_bottom = (dev->height - 1 - t) % 8;
            dev->buffer[page_top * dev->width + x] |= (1 << bit_top);
            dev->buffer[page_bottom * dev->width + x] |= (1 << bit_bottom);
        }
        // Linhas laterais
        for (int y = 0; y < dev->height; y++) {
            int page = y / 8;
            int bit = y % 8;
            dev->buffer[page * dev->width + (0 + t)] |= (1 << bit);
            dev->buffer[page * dev->width + (dev->width - 1 - t)] |= (1 << bit);
        }
    }
}
