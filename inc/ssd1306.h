#ifndef SSD1306_H
#define SSD1306_H

#include "hardware/i2c.h"

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_ADDRESS 0x3C

typedef struct {
    i2c_inst_t *i2c;
    uint8_t address;
    uint8_t width;
    uint8_t height;
    uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
} ssd1306_t;

void ssd1306_init(ssd1306_t *dev, i2c_inst_t *i2c, uint8_t address, uint8_t width, uint8_t height);
void ssd1306_show(ssd1306_t *dev);
void ssd1306_clear(ssd1306_t *dev);
void ssd1306_draw_pixel(ssd1306_t *dev, int x, int y, uint8_t color);
void ssd1306_draw_string(ssd1306_t *dev, int x, int y, const char *str);

#endif // SSD1306_H
