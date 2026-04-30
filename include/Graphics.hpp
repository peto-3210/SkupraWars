#pragma once
#include <stdint.h>

void draw_ship(int x, int y, uint16_t color);

// Nové funkce pro kreslení textu
void draw_char(int x, int y, char c, uint16_t color, uint16_t bg_color);
void draw_string(int x, int y, const char* str, uint16_t color, uint16_t bg_color);