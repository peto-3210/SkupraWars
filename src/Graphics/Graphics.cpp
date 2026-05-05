#include "Graphics/Graphics.hpp"
#include "Gameplay.hpp"
#include "hal/st7735.h"
#include <stdio.h> 
#include <avr/pgmspace.h>

// Definice barev
#define COLOR_GREEN		0x07E0
#define COLOR_BLUE		0x001F
#define COLOR_RED		0xF800
#define COLOR_WHITE		0xFFFF
#define COLOR_CYAN		0x07FF
#define COLOR_ORANGE	0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_BG        0x0000 // Černé pozadí

#define DISPLAY_LENGTH 160
#define DISPLAY_WIDTH 128

// Definice HUD regionů 
#define TOP_HUD_Y_START 0
#define TOP_HUD_Y_END   15
#define BOT_SEP_LINE_Y  140 // Červená čára
#define BOT_HUD_Y_START 141
#define SHIP_Y_TOP      124 // Horní okraj lodě. Kreslí se dolů k 140.

// Klasický 5x7 font (Znaky od ASCII 32 ' ' do 126 '~')
const uint8_t font5x7[] PROGMEM = {
	0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x2f,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7f,0x14,0x7f,0x14,
	0x24,0x2a,0x7f,0x2a,0x12, 0x23,0x13,0x08,0x64,0x62, 0x36,0x49,0x55,0x22,0x50, 0x00,0x05,0x03,0x00,0x00,
	0x00,0x1c,0x22,0x41,0x00, 0x00,0x41,0x22,0x1c,0x00, 0x14,0x08,0x3e,0x08,0x14, 0x08,0x08,0x3e,0x08,0x08,
	0x00,0x50,0x30,0x00,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x60,0x60,0x00,0x00, 0x20,0x10,0x08,0x04,0x02,
	0x3e,0x51,0x49,0x45,0x3e, 0x00,0x42,0x7f,0x40,0x00, 0x42,0x61,0x51,0x49,0x46, 0x21,0x41,0x45,0x4b,0x31,
	0x18,0x14,0x12,0x7f,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3c,0x4a,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03,
	0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1e, 0x00,0x36,0x36,0x00,0x00, 0x00,0x56,0x36,0x00,0x00,
	0x08,0x14,0x22,0x41,0x00, 0x14,0x14,0x14,0x14,0x14, 0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x51,0x09,0x06,
	0x32,0x49,0x79,0x41,0x3e, 0x7e,0x11,0x11,0x11,0x7e, 0x7f,0x49,0x49,0x49,0x36, 0x3e,0x41,0x41,0x41,0x22,
	0x7f,0x41,0x41,0x22,0x1c, 0x7f,0x49,0x49,0x49,0x41, 0x7f,0x09,0x09,0x09,0x01, 0x3e,0x41,0x49,0x49,0x7a,
	0x7f,0x08,0x08,0x08,0x7f, 0x00,0x41,0x7f,0x41,0x00, 0x20,0x40,0x41,0x3f,0x01, 0x7f,0x08,0x14,0x22,0x41,
	0x7f,0x40,0x40,0x40,0x40, 0x7f,0x02,0x0c,0x02,0x7f, 0x7f,0x04,0x08,0x10,0x7f, 0x3e,0x41,0x41,0x41,0x3e,
	0x7f,0x09,0x09,0x09,0x06, 0x3e,0x41,0x51,0x21,0x5e, 0x7f,0x09,0x19,0x29,0x46, 0x46,0x49,0x49,0x49,0x31,
	0x01,0x01,0x7f,0x01,0x01, 0x3f,0x40,0x40,0x40,0x3f, 0x1f,0x20,0x40,0x20,0x1f, 0x3f,0x40,0x38,0x40,0x3f,
	0x63,0x14,0x08,0x14,0x63, 0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43, 0x00,0x7f,0x41,0x41,0x00,
	0x02,0x04,0x08,0x10,0x20, 0x00,0x41,0x41,0x7f,0x00, 0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
	0x00,0x01,0x02,0x04,0x00, 0x20,0x54,0x54,0x54,0x78, 0x7f,0x48,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x20,
	0x38,0x44,0x44,0x48,0x7f, 0x38,0x54,0x54,0x54,0x18, 0x08,0x7e,0x09,0x01,0x02, 0x0c,0x52,0x52,0x52,0x3e,
	0x7f,0x08,0x04,0x04,0x78, 0x00,0x44,0x7d,0x40,0x00, 0x20,0x40,0x44,0x3d,0x00, 0x7f,0x10,0x28,0x44,0x00,
	0x00,0x41,0x7f,0x40,0x00, 0x7c,0x04,0x18,0x04,0x78, 0x7c,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38,
	0x7c,0x14,0x14,0x14,0x08, 0x08,0x14,0x14,0x18,0x7c, 0x7c,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x20,
	0x04,0x3f,0x44,0x40,0x20, 0x3c,0x40,0x40,0x20,0x7c, 0x1c,0x20,0x40,0x20,0x1c, 0x3c,0x40,0x30,0x40,0x3c,
	0x44,0x28,0x10,0x28,0x44, 0x0c,0x50,0x50,0x50,0x3c, 0x44,0x64,0x54,0x4c,0x44, 0x00,0x08,0x36,0x41,0x00,
	0x00,0x00,0x7f,0x00,0x00, 0x00,0x41,0x36,0x08,0x00, 0x10,0x08,0x08,0x10,0x08
};

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

/************************* draw_char *************************/
// Funkce, která nakreslí JEDEN znak podle naší ASCII tabulky nahoře
void draw_char(int x, int y, char c, uint16_t color, uint16_t bg_color) {
	if (c < 32 || c > 126) return; // Znaky mimo tabulku ignorujeme
	
	for (int i = 0; i < 5; i++) { // Font je široký 5 pixelů
		uint8_t line = pgm_read_byte(&font5x7[(c - 32) * 5 + i]);
		for (int j = 0; j < 8; j++) { // Výška znaku
			if (line & 0x1) {
				st7735_draw_pixel(x + i, y + j, color);
				} else {
				st7735_draw_pixel(x + i, y + j, bg_color);
			}
			line >>= 1;
		}
	}
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

/************************* draw_dotted_rect *************************/
// Vykreslí tečkovaný obrys
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
// Překreslí celý levý zásobník
void update_inventory_ui(uint8_t count, PowerUpType* inventory) {
    // 1. Smažeme celou oblast inventáře (počítám x=0 až 12, y=78 až 110)
    st7735_fill_rect(0, 78, 13, 33, COLOR_BG);
    
    // 2. Nakreslíme tečkovaný rámeček (12 pixelů široký, 34 vysoký)
    draw_dotted_rect(0, 78, 12, 33, COLOR_MAGENTA);

    // 3. Vykreslíme ikony, co hráč aktuálně má (LIFO)
    for (int i = 0; i < count; i++) {
        uint8_t draw_y = 80 + (i * 10);
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
		st7735_draw_pixel(DISPLAY_WIDTH/2, y, COLOR_MAGENTA);  
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
	draw_string(18, TOP_HUD_Y_START + 4, h1_s, COLOR_GREEN, COLOR_BG);
	draw_string(100, TOP_HUD_Y_START + 4, h2_s, COLOR_BLUE, COLOR_BG);
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
	draw_string(18, BOT_HUD_Y_START + 6, yh_s, COLOR_RED, COLOR_BG);

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
			draw_string(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, COLOR_WHITE, COLOR_BG);
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
			draw_string(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, COLOR_WHITE, COLOR_BG);
		}
		else if (w == WEP_LASER) {
			// Laser paprsek (široký, Cyan)
			st7735_fill_rect(slot_x + 8, BOT_HUD_Y_START + 3, 4, 8, COLOR_CYAN);
			
			// Ammo počet
			char ammo_s[3];
			ammo_s[0] = (ammo_counts[w] / 10) + '0';
			ammo_s[1] = (ammo_counts[w] % 10) + '0';
			ammo_s[2] = '\0';
			draw_string(slot_x + 5, BOT_HUD_Y_START + 11, ammo_s, COLOR_WHITE, COLOR_BG);
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
		draw_bitmap_PROGMEM(x, y, 8, 8, bmp, color, bg_color);
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