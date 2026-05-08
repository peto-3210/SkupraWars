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
#define DATA_SELECT_PIN PD7

// Makra pro rychlé přepínání stavu řídících pinů
#define ST7735_CS_LOW()  PORTB &= ~(1 << SPI_CS)
#define ST7735_CS_HIGH() PORTB |=  (1 << SPI_CS)

#define ST7735_DC_CMD()  PORTD &= ~(1 << DATA_SELECT_PIN) // Command mode
#define ST7735_DC_DATA() PORTD |=  (1 << DATA_SELECT_PIN) // Data mode

#define ST7735_RST_LOW()  PORTB &= ~(1 << RESET_PIN)
#define ST7735_RST_HIGH() PORTB |=  (1 << RESET_PIN)

// Define colors
#define COLOR_GREEN		0x07E0
#define COLOR_BLUE		0x001F
#define COLOR_RED		0xF800
#define COLOR_WHITE		0xFFFF
#define COLOR_CYAN		0x07FF
#define COLOR_ORANGE	0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_BG        0x0000


// 5x7 font (ASCII characters from 32 ' ' to 126 '~')
#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define ADD_BLANK_LINE 1 


// Init display
void st7735_init(void);

// Writes command to display
void st7735_write_command(uint8_t cmd);

// Writes data to display
void st7735_write_data(uint8_t data);

// Sets draw area, returns false if out of boundries
bool st7735_set_draw_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// Fills the entire screen with one color (565 RGB format)
void st7735_fill_screen(uint16_t color);

// Renders a rectangle
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

// Renders single pixel
void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color);

// Renders object from buffef
void st7735_draw_object(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t* buffer);

// Renders object defined by 8x8 bitmap
void draw_8bit_PROGMEM(uint8_t x, uint8_t y, const void *bitmap_array, uint16_t color, uint16_t bg_color);

// Renders object defined by 16x16 bitmap
void draw_16bit_PROGMEM(uint8_t x, uint8_t y, const void *bitmap_array, uint16_t color, uint16_t bg_color);

// Renders char array, counts as string when len is zero
void draw_char_buffer(int x, int y, const char* buffer, uint8_t len, uint16_t color, uint16_t bg_color);

#ifdef __cplusplus
    }
#endif

#endif // ST7735_H