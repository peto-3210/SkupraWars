#include "Graphics/Graphics.hpp"
#include "Gameplay.hpp"
#include "hal/st7735.h"
#include <stdio.h> 
#include <avr/pgmspace.h>

// HUD region definitions
#define TOP_HUD_Y_START 0
#define TOP_HUD_Y_END   15
#define BOT_SEP_LINE_Y  140 
#define BOT_HUD_Y_START 141
#define SHIP_Y_TOP      124 

// 1-bit spaceship mask
const uint16_t spaceship_mask[16] PROGMEM = {
    0x0180, 0x0180, 0x03C0, 0x03C0, 0x17E8, 0x17E8, 0x37EC, 0x3E7C,
    0x7E7E, 0x6FF6, 0xEFF7, 0x7BDE, 0x33CC, 0x03C0, 0x0180, 0x0000
};

// 8x8 pixel heart (1 bit per pixel = 8 bytes)
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

// ENEMY GOT HIT EMOTE
const uint8_t enemy_got_hit_emote_bitmap[] PROGMEM = {
	0b10000010, // *     * 
	0b01000100, //  *   *
	0b00101000, //   * *
	0b00010000, //    *
	0b00101000, //   * *      
	0b01000100, //  *   *
	0b10000010, // *     *
	0b00000000  //
};

/************************* draw_heart8x8 *************************/
// Helper function to render a heart
void draw_heart8x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color) {
    draw_8bit_PROGMEM(x, y, heart8x8_bitmap, color, bg_color);
}

/************************* draw_ship *************************/
// Renders the spaceship
void draw_ship(int x, int y, uint16_t color) {
    draw_16bit_PROGMEM(x, y, spaceship_mask, color, COLOR_BG);
}

/************************* draw_dotted_rect *************************/
// Renders a dotted outline
void draw_dotted_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t i = 0; i <= w; i += 2) {
        st7735_draw_pixel(x + i, y, color);
        st7735_draw_pixel(x + i, y + h, color);
    }
    for (int16_t i = 0; i <= h; i += 2) {
        st7735_draw_pixel(x, y + i, color);
        st7735_draw_pixel(x + w, y + i, color);
    }
}

/************************* update_inventory_ui *************************/
// Renders the entire left inventory UI
void update_inventory_ui(uint8_t count, PowerUpType* inventory) {
    // Clear the entire inventory area (assuming x=0 to 12, y=78 to 110)
    st7735_fill_rect(0, 78, 13, 33, COLOR_BG);
    
    // Draw a dotted frame (12 pixels wide, 34 pixels high)
    draw_dotted_rect(0, 78, 12, 33, COLOR_MAGENTA);

    // Draw the icons the player currently holds (LIFO)
    for (int i = 0; i < count; i++) {
        uint8_t draw_y = 100 - (i * 10);
        draw_powerup8x8(2, draw_y, inventory[i], COLOR_BG);
    }
}

/************************* gameplay_draw_top_hud_static *************************/
// Renders static elements of the Top HUD (not redrawn in the main loop)
void gameplay_draw_top_hud_static(void) {
	// Green separator line 
	st7735_fill_rect(0, TOP_HUD_Y_END, 128, 1, COLOR_GREEN);
	
	// Left side (Green heart)
	draw_heart8x8(5, TOP_HUD_Y_START + 4, COLOR_GREEN, COLOR_BG);
	
	// Right side (Blue heart)
	draw_heart8x8(115, TOP_HUD_Y_START + 4, COLOR_BLUE, COLOR_BG);
	
	// Center dotted line 
	for (uint8_t y = 0; y <= TOP_HUD_Y_END; y += 2) {
		st7735_draw_pixel(ST7735_WIDTH/2, y, COLOR_MAGENTA);  
	}
}

/************************* gameplay_draw_top_hud_dynamic *************************/
// Renders dynamic elements of the Top HUD (health indicators)
void gameplay_draw_top_hud_dynamic(uint8_t p1_health, uint8_t p2_health) {
	char h1_s[3]; // Buffer for 2 digits + null terminator
	char h2_s[3];
	
	// Health string formatting (P1: "99", P2: "99")
	h1_s[0] = (p1_health / 10) + '0';
	h1_s[1] = (p1_health % 10) + '0';
	h1_s[2] = '\0';
	
	h2_s[0] = (p2_health / 10) + '0';
	h2_s[1] = (p2_health % 10) + '0';
	h2_s[2] = '\0';

	// Renders text - colors based on team
	draw_char_buffer(18, TOP_HUD_Y_START + 4, h1_s, 0, COLOR_GREEN, COLOR_BG);
	draw_char_buffer(100, TOP_HUD_Y_START + 4, h2_s, 0, COLOR_BLUE, COLOR_BG);
}

/************************* draw_weapon_selection_box *************************/
// Renders a selection box around the weapon icon
void draw_weapon_selection_box(WeaponType wep, uint16_t color) {
	uint8_t slot_x = 43 + static_cast<int>(wep) * 21;
	uint8_t y = BOT_HUD_Y_START + 1;
	
	// Top line
	st7735_fill_rect(slot_x, y, 20, 1, color);
	// Bottom line
	st7735_fill_rect(slot_x, y + 17, 20, 1, color);
	// Left line
	st7735_fill_rect(slot_x, y, 1, 18, color);
	// Right line
	st7735_fill_rect(slot_x + 19, y, 1, 18, color);
}


/************************* gameplay_draw_bottom_hud *************************/
// Renders the bottom HUD (static elements + weapons)
void gameplay_draw_bottom_hud(uint8_t your_health, WeaponType active_weapon, uint8_t ammo_counts[]) {
	// Red separator line
	st7735_fill_rect(0, BOT_SEP_LINE_Y, 128, 1, COLOR_RED);
	
	// Left section: Red heart and health
	draw_heart8x8(5, BOT_HUD_Y_START + 6, COLOR_RED, COLOR_BG);
	char yh_s[3];
	yh_s[0] = (your_health / 10) + '0';
	yh_s[1] = (your_health % 10) + '0';
	yh_s[2] = '\0';
	draw_char_buffer(18, BOT_HUD_Y_START + 6, yh_s, 0, COLOR_RED, COLOR_BG);

	// Thick white separator column
	st7735_fill_rect(36, BOT_HUD_Y_START, 2, 160 - BOT_SEP_LINE_Y, COLOR_WHITE);

	// Weapons section (X=43 to 127)
	// We have 4 weapons. W=128-43=85px. 4 slots at ~21px each.

	for (int w = 0; w < 4; w++) {
		uint8_t slot_x = 43 + w * 21;
		
		// Highlight the active weapon
		uint16_t slot_border_color = (w == static_cast<int>(active_weapon)) ? COLOR_WHITE : COLOR_BG;
		draw_weapon_selection_box(static_cast<WeaponType>(w), slot_border_color);
		
		// --- Projectile Icons ---
		if (w == WEP_RAILGUN) {
			st7735_fill_rect(slot_x + 9, BOT_HUD_Y_START + 3, 2, 8, COLOR_BLUE);
			// Infinite ammo, displaying "--"
			st7735_fill_rect(slot_x + 4, BOT_HUD_Y_START + 13, 4, 2, COLOR_WHITE);
			st7735_fill_rect(slot_x + 10, BOT_HUD_Y_START + 13, 4, 2, COLOR_WHITE);
		}
		else if (w == WEP_BURST) {
			// Three vertical lines (Burst)
			st7735_fill_rect(slot_x + 5, BOT_HUD_Y_START + 3, 2, 8, COLOR_BLUE);
			st7735_fill_rect(slot_x + 9, BOT_HUD_Y_START + 3, 2, 8, COLOR_BLUE);
			st7735_fill_rect(slot_x + 13, BOT_HUD_Y_START + 3, 2, 8, COLOR_BLUE);
			
			// Ammo count
			char ammo_s[3];
			ammo_s[0] = (ammo_counts[w] / 10) + '0';
			ammo_s[1] = (ammo_counts[w] % 10) + '0';
			ammo_s[2] = '\0';
			draw_char_buffer(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, 0, COLOR_WHITE, COLOR_BG);
		}
		else if (w == WEP_ROCKET) {
			// Rocket shape 3x8 downwards
			st7735_fill_rect(slot_x + 9, BOT_HUD_Y_START + 3, 2, 8, COLOR_GREEN);   
			st7735_fill_rect(slot_x + 7, BOT_HUD_Y_START + 3, 2, 4, COLOR_ORANGE);  
			st7735_fill_rect(slot_x + 11, BOT_HUD_Y_START + 3, 2, 4, COLOR_ORANGE);
			
			// Ammo count
			char ammo_s[3];
			ammo_s[0] = (ammo_counts[w] / 10) + '0';
			ammo_s[1] = (ammo_counts[w] % 10) + '0';
			ammo_s[2] = '\0';
			draw_char_buffer(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, 0, COLOR_WHITE, COLOR_BG);
		}
		else if (w == WEP_LASER) {
			// Laser beam
			st7735_fill_rect(slot_x + 8, BOT_HUD_Y_START + 3, 4, 8, COLOR_CYAN);
			
			// Ammo count
			char ammo_s[3];
			ammo_s[0] = (ammo_counts[w] / 10) + '0';
			ammo_s[1] = (ammo_counts[w] % 10) + '0';
			ammo_s[2] = '\0';
			draw_char_buffer(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, 0, COLOR_WHITE, COLOR_BG);
		}
	}
}

/************************* draw_powerup8x8 *************************/
// Renders power-ups
void draw_powerup8x8(uint8_t x, uint8_t y, PowerUpType type, uint16_t bg_color) {
	const uint8_t* bmp = nullptr;
	uint16_t color = COLOR_WHITE;

	switch(type) {
		case PU_RAPID_FIRE:
			bmp = powerup_rapid_bitmap;
			color = COLOR_ORANGE;
			break;
		case PU_SHIELD:
			bmp = powerup_shield_bitmap;
			color = COLOR_CYAN;
			break;
		case PU_SENTRY:
			bmp = powerup_sentry_bitmap;
			color = COLOR_WHITE;
			break;
	}

	if (bmp != nullptr) {
		draw_8bit_PROGMEM(x, y, bmp, color, bg_color);
	}
}

/************************* draw_enemy_hit *************************/
// Renders X icon to indicate enemy hit
void draw_enemy_hit(bool direction){
	if (direction == false){
		draw_8bit_PROGMEM(48, 4, enemy_got_hit_emote_bitmap, COLOR_ORANGE, COLOR_BG);
	}
	else {
		draw_8bit_PROGMEM(72, 4, enemy_got_hit_emote_bitmap, COLOR_ORANGE, COLOR_BG);
	}
}

/************************* clear_enemy_hit *************************/
// Clears enemy_hit icon
void clear_enemy_hit(){
	st7735_fill_rect(48, 4, 8, 8, COLOR_BG);
	st7735_fill_rect(72, 4, 8, 8, COLOR_BG);
}