#pragma once
#include <avr/pgmspace.h>
#include <stdint.h>

#define MAX_INVENTORY 3
#define MAX_POWERUPS 4
#define MAX_SENTRIES 2


void draw_bitmap_PROGMEM(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap_array, uint16_t color, uint16_t bg_color);

void draw_heart8x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color);

void draw_infinity14x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color);

void draw_ship(int x, int y, uint16_t color);

void draw_char(int x, int y, char c, uint16_t color, uint16_t bg_color);

void draw_string(int x, int y, const char* str, uint16_t color, uint16_t bg_color);

// Kl��ov� slovo extern ��k�, �e pole je definovan� v jin�m souboru
extern const uint8_t powerup_rapid_bitmap[] PROGMEM;
extern const uint8_t powerup_shield_bitmap[] PROGMEM;
extern const uint8_t powerup_sentry_bitmap[] PROGMEM;
extern const uint8_t sentryGun_bitmap[] PROGMEM;