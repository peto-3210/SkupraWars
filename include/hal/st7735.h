#ifndef ST7735_H
#define ST7735_H

#include "hal/libraries.h"

#ifdef __cplusplus
    extern "C" {
#endif

// Makra pro rychlé přepínání stavu řídících pinů
#define ST7735_CS_LOW()  PORTB &= ~(1 << PB2)
#define ST7735_CS_HIGH() PORTB |=  (1 << PB2)

#define ST7735_DC_CMD()  PORTD &= ~(1 << PD0) // Režim příkazů
#define ST7735_DC_DATA() PORTD |=  (1 << PD0) // Režim dat

#define ST7735_RST_LOW()  PORTB &= ~(1 << PB0)
#define ST7735_RST_HIGH() PORTB |=  (1 << PB0)

// Základní inicializace displeje
void st7735_init(void);

// Odeslání příkazu do displeje
void st7735_write_command(uint8_t cmd);

// Odeslání dat do displeje
void st7735_write_data(uint8_t data);

// Vyplnění celého displeje jednou barvou (565 RGB formát)
void st7735_fill_screen(uint16_t color);

// Vykreslení menšího okna na zadaných souřadnicích
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

// Vykreslí jeden pixel na zadaných souřadnicích
void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color);

#ifdef __cplusplus
    }
#endif

#endif // ST7735_H