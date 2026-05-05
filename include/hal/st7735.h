#ifndef ST7735_H
#define ST7735_H

#include "hal/libraries.h"
#include "hal/spi.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define ST7735_WIDTH 128
#define ST7735_HEIGHT 160
#define RESET_PIN PB0
#define DATA_SELECT_PIN PD0

// Makra pro rychlé přepínání stavu řídících pinů
#define ST7735_CS_LOW()  PORTB &= ~(1 << SPI_CS)
#define ST7735_CS_HIGH() PORTB |=  (1 << SPI_CS)

#define ST7735_DC_CMD()  PORTD &= ~(1 << DATA_SELECT_PIN) // Režim příkazů
#define ST7735_DC_DATA() PORTD |=  (1 << DATA_SELECT_PIN) // Režim dat

#define ST7735_RST_LOW()  PORTB &= ~(1 << RESET_PIN)
#define ST7735_RST_HIGH() PORTB |=  (1 << RESET_PIN)

// Definice barev
#define COLOR_GREEN		0x07E0
#define COLOR_BLUE		0x001F
#define COLOR_RED		0xF800
#define COLOR_WHITE		0xFFFF
#define COLOR_CYAN		0x07FF
#define COLOR_ORANGE	0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_BG        0x0000 // Černé pozadí


// Klasický 5x7 font (Znaky od ASCII 32 ' ' do 126 '~')
#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define ADD_BLANK_LINE 1 //Prida prazdny riadok pod kazdy znak


// Základní inicializace displeje
void st7735_init(void);

// Odeslání příkazu do displeje
void st7735_write_command(uint8_t cmd);

// Odeslání dat do displeje
void st7735_write_data(uint8_t data);

// Nastavení kreslící oblasti (okna) pro následné kreslení
void st7735_set_draw_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// Vyplnění celého displeje jednou barvou (565 RGB formát)
void st7735_fill_screen(uint16_t color);

// Vykreslení menšího okna na zadaných souřadnicích
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

// Vykreslí jeden pixel na zadaných souřadnicích
void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color);

// Vykresli 1 znak
void draw_char(int x, int y, char c, uint16_t color, uint16_t bg_color);

// Vykresli pole znaku
void draw_buffer(int x, int y, const char* buffer, uint8_t len, uint16_t color, uint16_t bg_color);

#ifdef __cplusplus
    }
#endif

#endif // ST7735_H