#include "Graphics/Graphics.hpp"
#include "Gameplay.hpp"
#include "hal/st7735.h"
#include <stdio.h> 
#include <avr/pgmspace.h>

// Definice HUD regionů 
#define TOP_HUD_Y_START 0
#define TOP_HUD_Y_END   15
#define BOT_SEP_LINE_Y  140 // Červená čára
#define BOT_HUD_Y_START 141
#define SHIP_Y_TOP      124 // Horní okraj lodě. Kreslí se dolů k 140.


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


/************************* draw_heart8x8 *************************/
// Zkratka pro kreslení srdíčka
void draw_heart8x8(uint8_t x, uint8_t y, uint16_t color, uint16_t bg_color) {
    draw_8bit_PROGMEM(x, y, heart8x8_bitmap, color, bg_color);
}

// 1-bitová maska lodi
const uint16_t spaceship_mask[16] PROGMEM = {
    0x0180, 0x0180, 0x03C0, 0x03C0, 0x17E8, 0x17E8, 0x37EC, 0x3E7C,
    0x7E7E, 0x6FF6, 0xEFF7, 0x7BDE, 0x33CC, 0x03C0, 0x0180, 0x0000
};

/************************* draw_ship *************************/
// Funkce pro vykreslení lodě
void draw_ship(int x, int y, uint16_t color) {
    draw_16bit_PROGMEM(x, y, spaceship_mask, color, COLOR_BG);
}

/************************* draw_dotted_rect *************************/
// Vykreslí tečkovaný obrys
void draw_dotted_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {  
    // 1. Horní hrana (výška 1 pixel)
    st7735_fill_rect(x, y, w + 1, 1, color);
    
    // 2. Dolní hrana (výška 1 pixel)
    st7735_fill_rect(x, y + h, w + 1, 1, color);
    
    // 3. Levá hrana (šířka 1 pixel, od shora až dolů)
    st7735_fill_rect(x, y, 1, h + 1, color);
    
    // 4. Pravá hrana (šířka 1 pixel, od shora až dolů)
    st7735_fill_rect(x + w, y, 1, h + 1, color);
}

/************************* update_inventory_ui *************************/
// Překreslí celý levý zásobník
void update_inventory_ui(uint8_t count, PowerUpType* inventory) {
    // 1. Smažeme celou oblast inventáře (počítám x=0 až 12, y=78 až 110)
    st7735_fill_rect(0, 78, 13, 33, COLOR_BG);
    
    // 2. Nakreslíme tečkovaný rámeček (12 pixelů široký, 34 vysoký)
    draw_dotted_rect(0, 78, 12, 33, COLOR_MAGENTA);

    // 3. Vykreslíme ikony, co hráč aktuálně má (LIFO)
    for (int i = 0; i < count; i++) {
        uint8_t draw_y = 100 - (i * 10);
        draw_powerup8x8(2, draw_y, inventory[i], COLOR_BG);
    }
}

/************************* gameplay_draw_top_hud_static *************************/
// Vykreslí statické prvky Horního HUDu (nepřekresluje se v loopu)
void gameplay_draw_top_hud_static(void) {
	// 1. Zelená dělící čára (Y=15)
	st7735_fill_rect(0, TOP_HUD_Y_END, 128, 1, COLOR_GREEN);
	
	// 2. Levá strana (Zelené srdíčko)
	draw_heart8x8(5, TOP_HUD_Y_START + 4, COLOR_GREEN, COLOR_BG);
	
	// 3. Pravá strana (Modré srdíčko)
	draw_heart8x8(115, TOP_HUD_Y_START + 4, COLOR_BLUE, COLOR_BG);
	
	// 4. Středová tečkovaná čára 
	// Protože st7735 nemá dotted_line, musíme to udělat po pixelu
	for (uint8_t y = 0; y <= TOP_HUD_Y_END; y += 2) {
		st7735_draw_pixel(ST7735_WIDTH/2, y, COLOR_MAGENTA);  
	}
}

/************************* gameplay_draw_top_hud_dynamic *************************/
// Vykreslí dynamické prvky Horního HUDu (životy)
void gameplay_draw_top_hud_dynamic(uint8_t p1_health, uint8_t p2_health) {
	char h1_s[3]; // Buffer pro 2 číslice + terminator
	char h2_s[3];
	
	// Formátování životů (H1: "99", H2: "99")
	h1_s[0] = (p1_health / 10) + '0';
	h1_s[1] = (p1_health % 10) + '0';
	h1_s[2] = '\0';
	
	h2_s[0] = (p2_health / 10) + '0';
	h2_s[1] = (p2_health % 10) + '0';
	h2_s[2] = '\0';

	// Kreslení textu - barvy podle týmu
	// (Předpokládám, že draw_string umí smazat pozadí s bg_color)
	draw_char_buffer(18, TOP_HUD_Y_START + 4, h1_s, 0, COLOR_GREEN, COLOR_BG);
	draw_char_buffer(100, TOP_HUD_Y_START + 4, h2_s, 0, COLOR_BLUE, COLOR_BG);
}

/************************* draw_weapon_selection_box *************************/
// Nakreslí obdelní okolo zbraně
void draw_weapon_selection_box(WeaponType wep, uint16_t color) {
	uint8_t slot_x = 43 + static_cast<int>(wep) * 21;
	uint8_t y = BOT_HUD_Y_START + 1;
	
	// Horní linka
	st7735_fill_rect(slot_x, y, 20, 1, color);
	// Spodní linka
	st7735_fill_rect(slot_x, y + 17, 20, 1, color);
	// Levá linka
	st7735_fill_rect(slot_x, y, 1, 18, color);
	// Pravá linka
	st7735_fill_rect(slot_x + 19, y, 1, 18, color);
}

/************************* gameplay_draw_bottom_hud *************************/
// Vykreslí spodní HUD (statické prvky + zbraně)
void gameplay_draw_bottom_hud(uint8_t your_health, WeaponType active_weapon, uint8_t ammo_counts[]) {
	// 1. Červená dělící čára (Y=140)
	st7735_fill_rect(0, BOT_SEP_LINE_Y, 128, 1, COLOR_RED);
	
	// 2. Levá sekce: Červené srdce a životy
	draw_heart8x8(5, BOT_HUD_Y_START + 6, COLOR_RED, COLOR_BG);
	char yh_s[3];
	yh_s[0] = (your_health / 10) + '0';
	yh_s[1] = (your_health % 10) + '0';
	yh_s[2] = '\0';
	draw_char_buffer(18, BOT_HUD_Y_START + 6, yh_s, 0, COLOR_RED, COLOR_BG);

	// 3. Tlustý bílý dělící sloupec (začíná např. na X=36)
	st7735_fill_rect(36, BOT_HUD_Y_START, 2, 160 - BOT_SEP_LINE_Y, COLOR_WHITE); // Bílý, ať je vidět na černém pozadí

	// 4. Zbraně sekce (X=43 až 127)
	// Máme 4 zbraně. W=128-43=85px. 4 sloty po ~21px.

	for (int w = 0; w < 4; w++) {
		uint8_t slot_x = 43 + w * 21;
		
		// Zvýraznění aktivní zbraně pomocí naší nové funkce
		uint16_t slot_border_color = (w == static_cast<int>(active_weapon)) ? COLOR_WHITE : COLOR_BG;
		draw_weapon_selection_box(static_cast<WeaponType>(w), slot_border_color);
		
		// --- Ikonky Projektilů ---
		if (w == WEP_RAILGUN) {
			// "Rail gun" Text Labels (velmi malé fonty nemáme, skipujeme pro teď, kreslíme ikonu)
			st7735_fill_rect(slot_x + 9, BOT_HUD_Y_START + 3, 2, 8, COLOR_BLUE);
			// Ammo nekonečno, dáme "--" (dvě horizontální čárky 4x2 pixely)
			st7735_fill_rect(slot_x + 4, BOT_HUD_Y_START + 13, 4, 2, COLOR_WHITE);
			st7735_fill_rect(slot_x + 10, BOT_HUD_Y_START + 13, 4, 2, COLOR_WHITE);
		}
		else if (w == WEP_BURST) {
			// Tři čárky ( Burst)
			st7735_fill_rect(slot_x + 5, BOT_HUD_Y_START + 3, 2, 8, COLOR_BLUE);
			st7735_fill_rect(slot_x + 9, BOT_HUD_Y_START + 3, 2, 8, COLOR_BLUE);
			st7735_fill_rect(slot_x + 13, BOT_HUD_Y_START + 3, 2, 8, COLOR_BLUE);
			
			// Ammo počet
			char ammo_s[3];
			ammo_s[0] = (ammo_counts[w] / 10) + '0';
			ammo_s[1] = (ammo_counts[w] % 10) + '0';
			ammo_s[2] = '\0';
			draw_char_buffer(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, 0, COLOR_WHITE, COLOR_BG);
		}
		else if (w == WEP_ROCKET) {
			// Tvar rakety 3x8 dolů
			st7735_fill_rect(slot_x + 9, BOT_HUD_Y_START + 3, 2, 8, COLOR_GREEN);   // Zelené tělo
			st7735_fill_rect(slot_x + 7, BOT_HUD_Y_START + 3, 2, 4, COLOR_ORANGE);  // Oranžová křídla
			st7735_fill_rect(slot_x + 11, BOT_HUD_Y_START + 3, 2, 4, COLOR_ORANGE); // Oranžová křídla
			
			// Ammo počet
			char ammo_s[3];
			ammo_s[0] = (ammo_counts[w] / 10) + '0';
			ammo_s[1] = (ammo_counts[w] % 10) + '0';
			ammo_s[2] = '\0';
			draw_char_buffer(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, 0, COLOR_WHITE, COLOR_BG);
		}
		else if (w == WEP_LASER) {
			// Laser paprsek (široký, Cyan)
			st7735_fill_rect(slot_x + 8, BOT_HUD_Y_START + 3, 4, 8, COLOR_CYAN);
			
			// Ammo počet
			char ammo_s[3];
			ammo_s[0] = (ammo_counts[w] / 10) + '0';
			ammo_s[1] = (ammo_counts[w] % 10) + '0';
			ammo_s[2] = '\0';
			draw_char_buffer(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, 0, COLOR_WHITE, COLOR_BG);
		}
	}
}

/************************* draw_powerup8x8 *************************/
// Funkce pro kreslení power-ups
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
	0b10000001, // *      * 
	0b01000010, //  *    *
	0b00100100, //   *  *
	0b11111111, // ********
	0b00100100, //   *  *      
	0b01000010, //  *    *
	0b10000001, // *      *
	0b00000000  //
};