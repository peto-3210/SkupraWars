#include "Graphics.hpp"
#include "hal/st7735.h"
#include <avr/pgmspace.h>

// Srdíčko 8x8 pixelů (1 bit na pixel = 8 bajtů)
const uint8_t heart8x8_bitmap[] PROGMEM = {
	0b01100110, //  **  **
	0b11111111, // ********
	0b11111111, // ********
	0b11111111, // ********
	0b01111110, //  ******
	0b00111100, //   ****
	0b00011000, //    **
	0b00000000  // (padding)
};

/************************* draw_bitmap_PROGMEM *************************/
/**
 * @brief Vykreslí jednobarevnou bitmapu z paměti PROGMEM
 * * @param x X souřadnice levého horního rohu
 * @param y Y souřadnice levého horního rohu
 * @param w Šířka bitmapy v pixelech
 * @param h Výška bitmapy v pixelech
 * @param bitmap_array Ukazatel na pole bajtů v PROGMEM
 * @param color Barva ikonky
 * @param bg_color Barva pozadí ikonky (aby se smazalo staré srdíčko)
 */
void draw_bitmap_PROGMEM(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap_array, uint16_t color, uint16_t bg_color) {
    // Projdeme všechny řádky bitmapy
    for (uint8_t row = 0; row < h; row++) {
        // Počet bajtů na jeden řádek (w pixelů) je (w + 7) / 8
        uint8_t bytes_per_row = (w + 7) / 8;
        
        // Načteme bajty pro aktuální řádek z PROGMEM
        for (uint8_t byte_in_row = 0; byte_in_row < bytes_per_row; byte_in_row++) {
            uint8_t line = pgm_read_byte(&bitmap_array[row * bytes_per_row + byte_in_row]);
            
            // Projdeme všech 8 bitů v bajtu (nebo méně, pokud je w < 8)
            uint8_t bits_to_check = (byte_in_row == bytes_per_row - 1) ? (w % 8 == 0 ? 8 : w % 8) : 8;
            for (uint8_t bit = 0; bit < bits_to_check; bit++) {
                // Kontrolujeme bity od nejvýznamnějšího (vlevo)
                uint8_t px_x = x + byte_in_row * 8 + (7 - bit);
                if (line & (1 << (7 - bit))) {
                    st7735_draw_pixel(px_x, y + row, color); // Pixel zapnut (ikonka)
                } else {
                    st7735_draw_pixel(px_x, y + row, bg_color); // Pixel vypnut (pozadí)
                }
            }
        }
    }
}

/************************* draw_heart8x8 *************************/
// Zkratka pro kreslení srdíčka
void draw_heart8x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color) {
    draw_bitmap_PROGMEM(x, y, 8, 8, heart8x8_bitmap, color, bg_color);
}

/************************* draw_string *************************/
// Funkce, která projde celý text a nakreslí ho písmenko po písmenku
void draw_string(int x, int y, const char *str, uint16_t color, uint16_t bg_color) {
	while (*str) {
		draw_char(x, y, *str, color, bg_color);
		x += 6; // Posuneme se o 5 pixelů za znak + 1 pixel mezeru
		str++;
	}
}

// 1-bitová maska lodi
const uint16_t spaceship_mask[16] PROGMEM = {
    0x0180, 0x0180, 0x03C0, 0x03C0, 0x17E8, 0x17E8, 0x37EC, 0x3E7C,
    0x7E7E, 0x6FF6, 0xEFF7, 0x7BDE, 0x33CC, 0x03C0, 0x0180, 0x0000
};

/************************* draw_ship *************************/
// Funkce pro vykreslení lodě
void draw_ship(int x, int y, uint16_t color) {
    for (int i = 0; i < 16; i++) {
        uint16_t row_data = pgm_read_word(&spaceship_mask[i]);
        for (int j = 0; j < 16; j++) {
            if (row_data & (1 << (15 - j))) {
                st7735_draw_pixel(x + j, y + i, color);
            } else {
                st7735_draw_pixel(x + j, y + i, COLOR_BG);
            }
        }
    }
}

// RAPID FIRE - POWER UP
const uint8_t powerup_rapid_bitmap[] PROGMEM = {
	0b00111100, //   ****
	0b01000010, //  *    *
	0b10010001, // *  *   *
	0b10010001, // *  *   *
	0b10011001, // *  **  *
	0b10000001, // *      *
	0b01000010, //  *    *
	0b00111100  //   ****
};

// SHIELD - POWER UP
const uint8_t powerup_shield_bitmap[] PROGMEM = {
	0b01111110, //  ****** 
	0b11110001, // ****   *
	0b11110001, // ****   *
	0b11110001, // ****   *
	0b11110001, // ****   *
	0b01110010, //  ***  * 
	0b00111100, //   **** 
	0b00011000  //    **
};

// SENTRY GUN - POWER UP
const uint8_t powerup_sentry_bitmap[] PROGMEM = {
	0b00100000, //   *
	0b01111100, //  ***** 
	0b01100111, //  **  ***
	0b01111100, //  ***** 
	0b00010000, //    * 
	0b00111000, //   *** 
	0b01010100, //  * * * 
	0b10010010  // *  *  * 
};

// SENTRY GUN - OBJECT
const uint8_t sentryGun_bitmap[] PROGMEM = {
	0b00010000, //    *
	0b01111100, //  *****
	0b11101110, // *** ***
	0b01101100, //  ** **
	0b00111000, //   ***
	0b00111000, //   ***
	0b01010100, //  * * *
	0b10010010  // *  *  *
};