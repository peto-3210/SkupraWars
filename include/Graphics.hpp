#pragma once
#include <stdint.h>

void draw_heart8x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color);

void draw_infinity14x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color);

void draw_ship(int x, int y, uint16_t color);

void draw_char(int x, int y, char c, uint16_t color, uint16_t bg_color);

void draw_string(int x, int y, const char* str, uint16_t color, uint16_t bg_color);