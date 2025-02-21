#ifndef SSD1306_H
#define SSD1306_H

#include "hardware/i2c.h"
#include <stdint.h>

typedef struct {
    i2c_inst_t *i2c;
    uint8_t addr;
    uint8_t width;
    uint8_t height;
    uint8_t buffer[1024]; // Suporta até 128x64 pixels
} ssd1306_t;

void ssd1306_init(ssd1306_t *dev, i2c_inst_t *i2c, uint8_t addr, uint8_t width, uint8_t height);
void ssd1306_clear(ssd1306_t *dev);
void ssd1306_show(ssd1306_t *dev);
void ssd1306_draw_string(ssd1306_t *dev, uint8_t x, uint8_t y, const char *str);
void ssd1306_draw_border(ssd1306_t *dev, int thickness);

#endif
