#include "Gameplay.hpp"
#include "Graphics.hpp"
#include "Gamestate.hpp"
#include "Soundboard.hpp"
#include "SoftwareTimer.hpp"
#include "hal/st7735.h"
#include <avr/io.h>

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
    
    st7735_fill_screen(0xFFFF);
    draw_ship(x, SHIP_Y, 0x001F);
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
			st7735_draw_pixel(p.x, tail_y, 0xFFFF);
		}
		
		// Barvy podle toho, kdo střílí (Hráč: modrá stopa/světle modrá špička | Nepřítel: červená stopa/žlutá špička) 
		uint16_t trace_color = is_enemy ? 0xF800 : 0x001F;
		uint16_t tip_color   = is_enemy ? 0xFFE0 : 0x07FF; 

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
				st7735_draw_pixel(p.x, erase_y, 0xFFFF);
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
		st7735_fill_rect(p.x, erase_y, 3, 1, 0xFFFF);

		// Posun
		p.y += direction;

		// Rozlišení barev a umístění křidélek (aby raketa letěla špičkou dopředu)
		uint16_t body_color = is_enemy ? 0xF800 : 0x001F; // Hráč modrá, nepřítel červená 
		uint16_t wing_color = is_enemy ? 0x07FF : 0x07E0; // Hráč zelená, nepřítel světle modrá
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
			st7735_fill_rect(p.x, p.y, 3, 9, 0xFFFF);
			
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
			st7735_fill_rect(p.x, 16, 2, SHIP_Y - 16, 0xFFFF);
			} else {
			// Hráčův paprsek bude modrozelený, nepřátelský třeba čistě červený
			uint16_t laser_color = is_enemy ? 0xF800 : 0x07FF;
			st7735_fill_rect(p.x, 16, 2, SHIP_Y - 16, laser_color);
		}
	}
}

GameState gameplay_tick(void) {
	// Detekce stisku pro přepínání zbraní (jednoduchý debounce)
	static bool last_pd4_state = true;
	bool current_pd4_state = (PIND & (1 << PD4));

	if (!current_pd4_state && last_pd4_state) { // Detekce sestupné hrany (stisk)
		current_weapon = static_cast<WeaponType>((current_weapon + 1) % 4); 
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
			    // 1. Uklidíme "odpad"
			    
			    // A) Došlo k teleportu přes celou obrazovku? (Skok z 0 na 112 nebo naopak)
			    if ((old_x < 3 && x > 100) || (old_x > 100 && x < 3)) {
				    // Musíme smazat celou starou loď (16x16)
				    for (int dx = 0; dx < SHIP_LENGTH; dx++) {
					    for (int dy = 0; dy < SHIP_LENGTH; dy++) {
						    st7735_draw_pixel(old_x + dx, SHIP_Y + dy, 0xFFFF);
					    }
				    }
			    }
			    // B) Normální posun doleva (mažeme pravý okraj)
			    else if (x < old_x) {
				    for (int i = 0; i < SHIP_LENGTH; i++) {
					    st7735_draw_pixel(old_x + 13, SHIP_Y + i, 0xFFFF);
					    st7735_draw_pixel(old_x + 14, SHIP_Y + i, 0xFFFF);
					    st7735_draw_pixel(old_x + 15, SHIP_Y + i, 0xFFFF);
				    }
			    }
			    // C) Normální posun doprava (mažeme levý okraj)
			    else {
				    for (int i = 0; i < SHIP_LENGTH; i++) {
					    st7735_draw_pixel(old_x, SHIP_Y + i, 0xFFFF);
					    st7735_draw_pixel(old_x + 1, SHIP_Y + i, 0xFFFF);
					    st7735_draw_pixel(old_x + 2, SHIP_Y + i, 0xFFFF);
				    }
			    }
			    
			    // 2. Vykreslíme loď na nové pozici
			    draw_ship(x, SHIP_Y, 0x001F);
		    }
	    }
	    last_A = current_A;
    }	
	
	return STATE_GAMEPLAY;
}

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

