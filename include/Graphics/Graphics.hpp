#pragma once
#include <avr/pgmspace.h>
#include <stdint.h>

#define MAX_INVENTORY 3
#define MAX_POWERUPS 4
#define MAX_SENTRIES 1

// Draw heart icon
void draw_heart8x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color);

// Draw ship
void draw_ship(int x, int y, uint16_t color);

// Extern bitmaps defined in Graphics.cpp
extern const uint8_t powerup_rapid_bitmap[] PROGMEM;
extern const uint8_t powerup_shield_bitmap[] PROGMEM;
extern const uint8_t powerup_sentry_bitmap[] PROGMEM;
extern const uint8_t sentryGun_bitmap[] PROGMEM;