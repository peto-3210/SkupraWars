#pragma once
#include <avr/pgmspace.h>
#include <stdint.h>

#define MAX_INVENTORY 3
#define MAX_POWERUPS 4
#define MAX_SENTRIES 1

/**
 * @brief Specialized helper to render a 1-bit heart icon.
 * * Simplifies the call to the generic `draw_8bit_PROGMEM` function by 
 * automatically referencing the `heart8x8_bitmap`. This icon is used 
 * throughout the HUD to represent health points (HP) for both player 
 * and enemies.
 * * @param x,y Top-left coordinates for the icon.
 * @param color The 16-bit color for the heart shape (e.g., COLOR_RED).
 * @param bg_color The 16-bit color for the surrounding 8x8 area.
 */
void draw_heart8x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color);


/**
 * @brief Specialized helper to render the player's spaceship.
 * * Handles the visual representation of the player entity by calling 
 * `draw_16bit_PROGMEM` with the `spaceship_mask` bitmap. 
 * - **Transparency:** Uses `COLOR_BG` for the background bits, effectively 
 * erasing previous positions or blending with the game's background.
 * - **Customization:** The `color` parameter allows for dynamic changes, 
 * such as flashing when hit or changing color based on power-ups.
 * * @param x,y Top-left coordinates of the 16x16 pixel spaceship.
 * @param color The 16-bit RGB565 color used for the ship's body.
 */
void draw_ship(int x, int y, uint16_t color);


// Extern bitmaps defined in Graphics.cpp
extern const uint8_t powerup_rapid_bitmap[] PROGMEM;
extern const uint8_t powerup_shield_bitmap[] PROGMEM;
extern const uint8_t powerup_sentry_bitmap[] PROGMEM;
extern const uint8_t sentryGun_bitmap[] PROGMEM;