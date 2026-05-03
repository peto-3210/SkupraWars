#include "Gameplay.hpp"
#include "Graphics.hpp"
#include "Gamestate.hpp"
#include "Soundboard.hpp"
#include "SoftwareTimer.hpp"
#include "hal/st7735.h"
#include <avr/io.h>

// --- Definice HUD regionů a barev ---
#define TOP_HUD_Y_START 0
#define TOP_HUD_Y_END   15

#define BOT_SEP_LINE_Y  140 // Červená čára
#define BOT_HUD_Y_START 141

#define SHIP_Y_TOP      124 // Horní okraj lodě. Kreslí se dolů k 140.

// Definice barev z nákresu
#define COLOR_GREEN		0x07E0 
#define COLOR_BLUE		0x001F
#define COLOR_RED		0xF800 
#define COLOR_WHITE		0xFFFF 
#define COLOR_CYAN		0x07FF
#define COLOR_ORANGE	0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_BG        0x0000 // Černé pozadí

#define MAX_LASERS 7
#define LASER_LENGTH 5
#define SHIP_LENGTH 16
#define DISPLAY_LENGTH 160
#define DISPLAY_WIDTH 128
#define COOLDOWN_NORMAL 600000  // 0.6 sekundy
#define COOLDOWN_RAPID  300000  // 0.3 sekundy 

enum WeaponType {
	WEP_RAILGUN = 0,
	WEP_BURST = 1,
	WEP_ROCKET = 2,
	WEP_LASER = 3
};

// Aktuálně zvolená zbraň
WeaponType current_weapon = WEP_RAILGUN;

// Struktura projektilů
struct Projectile {
	bool active;
	uint8_t x, y;
	WeaponType type;
	uint32_t spawn_time; // Důležité pro 3sekundový laser
};

// Globální stav nábojů hráče
// Indexy odpovídají: 0=RAILGUN, 1=BURST, 2=ROCKET, 3=LASER
uint8_t player_ammo[4] = {
	255, // WEP_RAILGUN: Hodnota je fuk, HUD kreslí nekonečno a v logice ho nebudeme odečítat
	15,  // WEP_BURST: Startovní počet nábojů
	5,   // WEP_ROCKET: Startovní počet raket
	10   // WEP_LASER: Startovní počet laserů
};

// Pomocné proměnné pro Burst (Dávkovaný railgun)
uint8_t burst_shots_left = 0;
SoftwareTimer* burstTimer = nullptr; 

// --- Lokální proměnné pro hru ---
static Projectile player_lasers[MAX_LASERS];
static Projectile enemy_lasers[MAX_LASERS];
static uint8_t x = DISPLAY_WIDTH/2; // Ship is positioned in the middle of the screen
static const uint8_t SHIP_Y = DISPLAY_LENGTH - 36;
static uint8_t last_A;

typedef enum { BTN_IDLE,
	           BTN_WAIT_DEBOUNCE,
	           BTN_WAIT_RELEASE,
	           BTN_RELEASE_DEBOUNCE } ButtonState;
static ButtonState btn_state = BTN_IDLE;

static SoftwareTimer* btnTimer = nullptr;
static SoftwareTimer* projectileTimer = nullptr;
static SoftwareTimer* fireCooldownTimer = nullptr;
static bool isRapidFireActive = false; // Zatím false, později se bude měnit po sebrání power-upu

// --- Funkce pro vykreslení HUD ---

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

	// 3. Tlustý bílý dělící sloupec (začíná např. na X=40)
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

// Funkce přijme referenci na JAKOUKOLIV střelu a informaci, čí ta střela je
void process_projectile(Projectile& p, bool is_enemy) {
	
	// Zjistíme směr a hranici zániku podle toho, kdo střílí
	int direction = is_enemy ? 1 : -1;       // Nepřítel přičítá Y (+1), hráč odečítá (-1)
	int edge_limit = is_enemy ? SHIP_Y : 16; // Kam až může letět
	
	// --- 1. RAILGUN & BURST ---
	if (p.type == WEP_RAILGUN || p.type == WEP_BURST) {
		
		// Zjištění pozice ocasu střely
		int tail_offset = is_enemy ? -LASER_LENGTH : LASER_LENGTH;
		int tail_y = p.y + tail_offset;
		
		// Smažeme ocas střely, pokud je ještě na obrazovce
		bool tail_on_screen = is_enemy ? (tail_y > 16) : (tail_y < SHIP_Y);
		if (tail_on_screen) {
			st7735_draw_pixel(p.x, tail_y, COLOR_BG);
		}
		
		// Barvy podle toho, kdo střílí (Hráč: modrá stopa/světle modrá špička | Nepřítel: červená stopa/žlutá špička)
		uint16_t trace_color = is_enemy ? COLOR_RED : COLOR_BLUE;
		uint16_t tip_color   = is_enemy ? COLOR_ORANGE : COLOR_CYAN;

		// Stará špička ztmavne
		st7735_draw_pixel(p.x, p.y, trace_color);
		
		// Posun
		p.y += direction;
		
		// Nová zářivá špička
		st7735_draw_pixel(p.x, p.y, tip_color);

		// Kontrola okrajů
		bool out_of_bounds = is_enemy ? (p.y >= edge_limit) : (p.y <= edge_limit);
		if (out_of_bounds) {
			p.active = false;
			
			// Smazání celého laseru z obrazovky
			for(uint8_t j = 0; j <= LASER_LENGTH; j++) {
				int erase_y = is_enemy ? (p.y - j) : (p.y + j);
				st7735_draw_pixel(p.x, erase_y, COLOR_BG);
			}
			
			// Odeslání přes UART (odesíláme logicky jen vlastní střely)
			if (!is_enemy) {
				// uart_send_projectile(p.x, p.type);
			}
		}
	}
	
	// --- 2. RAKETOMET ---
	else if (p.type == WEP_ROCKET) {
		
		// Smazání stopy po raketě (spodní/horní řádek, který po posunu zůstane)
		int erase_y = is_enemy ? (p.y - 1) : (p.y + 8);
		st7735_fill_rect(p.x, erase_y, 3, 1, COLOR_BG);

		// Posun
		p.y += direction;

		// Rozlišení barev a umístění křidélek (aby raketa letěla špičkou dopředu)
		uint16_t body_color = is_enemy ? COLOR_RED : COLOR_GREEN; // Hráč zelená, nepřítel červená
		uint16_t wing_color = is_enemy ? COLOR_BLUE : COLOR_ORANGE; // Hráč oranžová, nepřítel modrá
		int wing_y_offset   = is_enemy ? 0 : 4;           // Křidélka vzadu

		// Vykreslení rakety na nové pozici (x, y, šířka, výška)
		st7735_fill_rect(p.x + 1, p.y, 1, 8, body_color);                    // Tělo
		st7735_fill_rect(p.x, p.y + wing_y_offset, 1, 4, wing_color);        // Levé křidélko
		st7735_fill_rect(p.x + 2, p.y + wing_y_offset, 1, 4, wing_color);    // Pravé křidélko

		// Kontrola okrajů
		bool out_of_bounds = is_enemy ? (p.y >= edge_limit) : (p.y <= edge_limit);
		if (out_of_bounds) {
			p.active = false;
			// Smazání celé 3x8 rakety
			st7735_fill_rect(p.x, p.y, 3, 9, COLOR_BG);
			
			if (!is_enemy) {
				// uart_send_projectile(p.x, p.type);
			}
		}
	}
	
	// --- 3. LASER ---
	else if (p.type == WEP_LASER) {
		// Paprsek se v ose Y neposouvá, jen trvá 3 sekundy
		if (micros() - p.spawn_time > 3000000UL) {
			p.active = false;
			// Vypršel čas -> smažeme paprsek. Vždy je od y=16 až k lodi (SHIP_Y).
			st7735_fill_rect(p.x, 16, 2, SHIP_Y - 16, COLOR_BG);
			} else {
			// Hráčův paprsek bude modrozelený, nepřátelský třeba čistě červený
			uint16_t laser_color = is_enemy ? COLOR_RED : COLOR_CYAN;
			st7735_fill_rect(p.x, 16, 2, SHIP_Y - 16, laser_color);
		}
	}
}

void gameplay_init(void) {
    btnTimer = SoftwareTimerPool::acquireTimer();
    projectileTimer = SoftwareTimerPool::acquireTimer();
	burstTimer = SoftwareTimerPool::acquireTimer();
	burstTimer->startTimerUs(100);
	fireCooldownTimer = SoftwareTimerPool::acquireTimer();
	fireCooldownTimer->startTimerUs(0);
    
    last_A = (PIND & (1 << PD2)) >> PD2;
    x = DISPLAY_WIDTH/2; // Reset pozice
    
    for (int i = 0; i < MAX_LASERS; i++) {
        player_lasers[i].active = false;
        enemy_lasers[i].active = false;
    }
    
    // 1. Vyplníme celou obrazovku černě (vesmír)
    st7735_fill_screen(COLOR_BG);

    // 2. Vykreslíme statické lišty a ikonky
    gameplay_draw_top_hud_static();
    
    // 3. Vykreslíme dynamicé prvky se startovními hodnotami
    gameplay_draw_top_hud_dynamic(99, 99); // P1_H, P2_H
    
    // Použijeme startovní zbraň a ammo (vytvoř si pole s ammo např. na začátku gameplay.cpp)
    gameplay_draw_bottom_hud(99, WEP_RAILGUN, player_ammo); // your_hp, wep, ammo_array

    // 4. Kreslíme loď na nové pozici SHIP_Y_TOP=124 (kreslí se dolů k 140)
    st7735_fill_rect(x, SHIP_Y_TOP, SHIP_LENGTH, SHIP_LENGTH, COLOR_BLUE);
}

GameState gameplay_tick(void) {
	// Detekce stisku pro přepínání zbraní (jednoduchý debounce)
	static bool last_pd4_state = true;
	bool current_pd4_state = (PIND & (1 << PD4));

	if (!current_pd4_state && last_pd4_state) { // Detekce sestupné hrany (stisk)
		// 1. Zapamatujeme si starou zbraň
		WeaponType old_weapon = current_weapon;
		
		// 2. Přepneme na další
		current_weapon = static_cast<WeaponType>((current_weapon + 1) % 4);
		
		// 3. Smažeme rámeček kolem staré zbraně
		draw_weapon_selection_box(old_weapon, COLOR_BG);
		
		// 4. Vykreslíme rámeček kolem nové zbraně
		draw_weapon_selection_box(current_weapon, COLOR_WHITE);
	}
	last_pd4_state = current_pd4_state;
	
    // --- ČTENÍ TLAČÍTKA (STŘELBA) ---
    switch (btn_state) {
	    case BTN_IDLE:
	    // PŘIDÁNA KONTROLA COOLDOWNU:
	    if (!(PIND & (1 << PD5)) && fireCooldownTimer->isDone()) {
		    btnTimer->startTimerUs(100000);
		    btn_state = BTN_WAIT_DEBOUNCE;
	    }
	    break;

	    case BTN_WAIT_DEBOUNCE:
	    if (btnTimer->isDone()) {
		    if (!(PIND & (1 << PD5))) { // Pokud stisknuto tlačítko střelby
			    
			    // 1. Zpracování okamžité střelby
			    if (current_weapon == WEP_RAILGUN || current_weapon == WEP_ROCKET || current_weapon == WEP_LASER) {
				    for (int i = 0; i < MAX_LASERS; i++) {
					    if (!player_lasers[i].active) {
						    player_lasers[i].active = true;
						    player_lasers[i].x = x + 7; // Vycentrování
						    player_lasers[i].y = (current_weapon == WEP_ROCKET) ? (SHIP_Y - 8) : (SHIP_Y - 1);
						    player_lasers[i].type = current_weapon;
						    player_lasers[i].spawn_time = micros(); // Zaznamenáme čas výstřelu
						    
						    // Přehrajeme správný zvuk
						    if (current_weapon == WEP_RAILGUN) Soundboard::playSound(Soundboard::sfx_railgun);
						    if (current_weapon == WEP_ROCKET) Soundboard::playSound(Soundboard::sfx_rocket);
						    if (current_weapon == WEP_LASER) Soundboard::playSound(Soundboard::sfx_laser);
						    
						    break; // Vystřelí jen jeden
					    }
				    }
			    }
			    // 2. Zpracování Burst zbraně (jen zahájí dávku)
			    else if (current_weapon == WEP_BURST) {
				    burst_shots_left = 3; // Chceme vystřelit 3x
				    burstTimer->startTimerUs(0); // První vyletí hned
			    }
			    
			    // Timer sdílí všechny střely, takže ho pustíme, pokud ještě neběží
			    projectileTimer->startTimerUs(80000);
			    
			    // Nastartování cooldownu podle toho, jestli máme Power-up
			    uint32_t current_cd = isRapidFireActive ? COOLDOWN_RAPID : COOLDOWN_NORMAL;
			    fireCooldownTimer->startTimerUs(current_cd);
			    
			    btn_state = BTN_WAIT_RELEASE;
			    } else {
			    btn_state = BTN_IDLE; // Falešný stisk (tlačítko bylo puštěno moc brzo)
		    }
	    }
	    break;

	    case BTN_WAIT_RELEASE:
	    if (PIND & (1 << PD5)) {
		    btnTimer->startTimerUs(50000);
		    btn_state = BTN_RELEASE_DEBOUNCE;
	    }
	    break;

	    case BTN_RELEASE_DEBOUNCE:
	    if (btnTimer->isDone()) {
		    btn_state = BTN_IDLE;
	    }
	    break;
    } // <-- ZDE KONČÍ SWITCH TLAČÍTKA

    // --- LOGIKA PRO DOKONČENÍ BURST DÁVKY (Nyní běží volně v gameplay_tick) ---
    if (burst_shots_left > 0 && burstTimer->isDone()) {
	    for (int i = 0; i < MAX_LASERS; i++) {
		    if (!player_lasers[i].active) {
			    player_lasers[i].active = true;
			    player_lasers[i].x = x + 7;
			    player_lasers[i].y = (current_weapon == WEP_ROCKET) ? (SHIP_Y - 8) : (SHIP_Y - 1);
			    player_lasers[i].type = WEP_RAILGUN; // Burst střílí obyčejné Railgun projektily
			    
			    Soundboard::playSound(Soundboard::sfx_burst);
			    break;
		    }
	    }
	    burst_shots_left--;
	    if (burst_shots_left > 0) {
		    burstTimer->startTimerUs(250000); // 250 ms mezera mezi střelami v dávce
	    }
    }

    // --- FYZIKA A VYKRESLOVÁNÍ PROJEKTILŮ ---
    if (projectileTimer->isDone()) {
	    bool any_laser_active = false;

		// A) HRÁČOVY STŘELY
		for (int i = 0; i < MAX_LASERS; i++) {
			if (player_lasers[i].active) {
				any_laser_active = true;
				process_projectile(player_lasers[i], false); // false = není to nepřítel
			}
		}

		// B) NEPŘÁTELSKÉ STŘELY
		for (int i = 0; i < MAX_LASERS; i++) {
			if (enemy_lasers[i].active) {
				any_laser_active = true;
				process_projectile(enemy_lasers[i], true); // true = je to nepřítel
			}
		}

	    // Pokud letí alespoň jeden laser (můj nebo cizí), restartujeme časovač
	    if (any_laser_active) {
		    projectileTimer->startTimerUs(25000);
	    }
    }

    // --- ČTENÍ ENKODÉRU (POHYB LODĚ) ---
    uint8_t current_A = (PIND & (1 << PD3)) >> PD3;

    if (current_A != last_A) {
	    if (current_A == 0) {
		    uint8_t current_B = (PIND & (1 << PD2)) >> PD2;
		    uint8_t old_x = x; // Zapamatujeme si starou pozici

		    // --- Výpočet nové pozice s wrappingem ---
		    if (current_B != current_A) {
			    if (x >= 3) {
				    x -= 3; // Doleva
				    } else {
				    x = DISPLAY_WIDTH - SHIP_LENGTH; // Skok na pravý okraj (128 - 16 = 112)
			    }
			    } else {
			    if (x <= (DISPLAY_WIDTH - SHIP_LENGTH - 3)) {
				    x += 3; // Doprava
				    } else {
				    x = 0; // Skok na levý okraj
			    }
		    }

		    // Pokud jsme se pohnuli, překreslíme
		    if (x != old_x) {
			    // 1. Uklidíme "odpad" pomocí rychlého fill_rect!
			    
			    // A) Teleport (Skok přes celou obrazovku)
			    if ((old_x < 3 && x > 100) || (old_x > 100 && x < 3)) {
				    // Smažeme celý starý prostor 16x16
				    st7735_fill_rect(old_x, SHIP_Y, SHIP_LENGTH, SHIP_LENGTH, COLOR_BG);
			    }
			    // B) Normální posun doleva (mažeme 3 pixely široký pruh vpravo)
			    else if (x < old_x) {
				    st7735_fill_rect(old_x + 13, SHIP_Y, 3, SHIP_LENGTH, COLOR_BG);
			    }
			    // C) Normální posun doprava (mažeme 3 pixely široký pruh vlevo)
			    else {
				    st7735_fill_rect(old_x, SHIP_Y, 3, SHIP_LENGTH, COLOR_BG);
			    }
			    
			    // 2. Vykreslíme loď na nové pozici 
			    draw_ship(x, SHIP_Y, COLOR_BLUE);
		    }
	    }
	    last_A = current_A;
    }	
	
	return STATE_GAMEPLAY;
}

void try_shoot(WeaponType wep) {
	// 1. Kontrola, zda máme náboje (nebo zda je to Railgun s nekonečnem)
	if (wep == WEP_RAILGUN || player_ammo[wep] > 0) {
		
		// ... Zde zavolat logiku pro vytvoření samotné střely ...
		// spawn_projectile(wep, x, y);

		// 2. Odečtení náboje (pokud to není Railgun)
		if (wep != WEP_RAILGUN) {
			player_ammo[wep]--;
		}
		
		// 3. Překreslení HUDu (aby se číslo hned zaktualizovalo)
		// Pozor: Překreslovat celý HUD při každém výstřelu může být pomalé.
		// Ideální je později napsat funkci, která přepíše jen to jedno konkrétní číslo na obrazovce.
		} else {
		// Hráč nemá náboje -> můžeš přehrát zvuk naprázdno "cvak"
	}
}

void add_ammo(WeaponType wep, uint8_t amount) {
	// Přičteme náboje
	player_ammo[wep] += amount;
	
	// Omezíme na max 99 kvůli vykreslování na displeji
	if (player_ammo[wep] > 99) {
		player_ammo[wep] = 99;
	}
}

// Použití ve hře:
// add_ammo(WEP_ROCKET, 5);

void gameplay_spawn_enemy_laser(uint8_t received_x) {
	uint8_t mirrored_x = 127 - received_x;
	for (int i = 0; i < MAX_LASERS; i++) {
		if (!enemy_lasers[i].active) {
			enemy_lasers[i].active = true;
			enemy_lasers[i].x = mirrored_x;
			enemy_lasers[i].y = 16;
			
			// NEZAPOMENOUT NASTAVIT TYP!
			enemy_lasers[i].type = WEP_RAILGUN;
			
			break;
		}
	}
}

