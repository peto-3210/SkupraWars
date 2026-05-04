#include "Gameplay.hpp"
#include "Graphics.hpp"
#include "Gamestate.hpp"
#include "Soundboard.hpp"
#include "SoftwareTimer.hpp"
#include "hal/st7735.h"
#include <avr/io.h>
#include <stdlib.h>

#define DISPLAY_LENGTH 160
#define DISPLAY_WIDTH 128

// Definice HUD regionů 
#define TOP_HUD_Y_START 0
#define TOP_HUD_Y_END   15
#define BOT_SEP_LINE_Y  140 // Červená čára
#define BOT_HUD_Y_START 141
#define SHIP_Y_TOP      124 // Horní okraj lodě. Kreslí se dolů k 140.

// Definice barev
#define COLOR_GREEN		0x07E0 
#define COLOR_BLUE		0x001F
#define COLOR_RED		0xF800 
#define COLOR_WHITE		0xFFFF 
#define COLOR_CYAN		0x07FF
#define COLOR_ORANGE	0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_BG        0x0000 // Černé pozadí

#define MAX_PROJECTILES 17
#define LASER_LENGTH 5
#define SHIP_LENGTH 16

// Řešení power ups
#define MAX_INVENTORY 3
#define MAX_POWERUPS 4
#define MAX_SENTRIES 2
#define POWERUP_HOLD_TIME 600000  // 600 ms pro dlouhý stisk
#define LIFETIME_RAPID 5000000    // 5 sec
#define LIFETIME_SHIELD 5000000   // 5 sec
#define LIFETIME_SENTRY 10000000  // 10.0 sec
#define COOLDOWN_SENTRY 1000000   // 1.0 sec


/************* ENUMY *************/
typedef enum { BTN_IDLE,
	BTN_WAIT_DEBOUNCE,
	BTN_WAIT_RELEASE,
	BTN_WAIT_FULL_RELEASE,
BTN_RELEASE_DEBOUNCE } ButtonState;

enum PowerUpType {
	PU_RAPID_FIRE,
	PU_SHIELD,
	PU_SENTRY
};

enum WeaponType {
	WEP_RAILGUN = 0,
	WEP_BURST = 1,
	WEP_ROCKET = 2,
	WEP_LASER = 3
};

/************* STRUKTURY *************/
struct SentryGun {
	bool active;
	uint8_t x;
	uint8_t y;
	uint32_t spawn_time;      // Pro odpočet smazání věže
	uint32_t last_shot_time;  // Pro časování střelby
};

struct Projectile {
	bool active;
	uint8_t x, y;
	WeaponType type;
	uint32_t spawn_time;
};

struct PowerUp {
	bool active;
	uint8_t x;
	uint8_t y;
	PowerUpType type;
	uint8_t health;
	uint32_t spawn_time;
};

// Časy jsou v mikrosekundách
const uint32_t COOLDOWNS_NORMAL[4] = {
	600000,  // WEP_RAILGUN  (0.6 sec)
	700000,  // WEP_BURST    (0.7 sec)
	1000000, // WEP_ROCKET   (1.0 sec)
	800000   // WEP_LASER    (0.8 sec)
};

const uint32_t COOLDOWNS_RAPID[4] = {
	400000,  // WEP_RAILGUN  (0.4 sec)
	450000,  // WEP_BURST    (0.45 sec)
	700000, // WEP_ROCKET    (0.7 sec)
	500000   // WEP_LASER    (0.5 sec)
};

// Globální stav nábojů hráče
// Indexy odpovídají: 0=RAILGUN, 1=BURST, 2=ROCKET, 3=LASER
uint8_t player_ammo[4] = {
	255,  // WEP_RAILGUN: Hodnota je fuk, HUD kreslí nekonečno a v logice ho nebudeme odečítat
	99,   // WEP_BURST: Startovní počet nábojů
	99,   // WEP_ROCKET: Startovní počet raket
	99    // WEP_LASER: Startovní počet laserů
};

PowerUpType player_inventory[MAX_INVENTORY];
SentryGun active_sentries[MAX_SENTRIES];
PowerUp active_powerups[MAX_POWERUPS];

// --- Lokální proměnné pro hru ---
static Projectile player_lasers[MAX_PROJECTILES];
static Projectile enemy_lasers[MAX_PROJECTILES];
static const uint8_t SHIP_Y = DISPLAY_LENGTH - 36;







// Alokace timerů
static SoftwareTimer* btnTimer = nullptr;
static SoftwareTimer* projectileTimer = nullptr;
static SoftwareTimer* fireCooldownTimer = nullptr;
static SoftwareTimer* holdTimer = nullptr;
static SoftwareTimer* rapidFirePUTimer = nullptr;
static SoftwareTimer* shieldPUTimer = nullptr;
static SoftwareTimer* powerupSpawnTimer = nullptr;
static SoftwareTimer* burstTimer = nullptr; 

// Globální proměnné pro stavy (Sentry asi bude mít vlastní strukturu)
bool isShieldActive;
bool isRapidFireActive; 
WeaponType current_weapon;
static ButtonState btn_state;
static uint8_t last_A;
uint8_t inventory_count;
static uint8_t x; // Pozice lodě
uint8_t burst_shots_left;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

/************************* add_powerup_to_inventory *************************/
// Funkce pro přidání power-up do zásobníku
void add_powerup_to_inventory(PowerUpType type) {
	if (inventory_count < MAX_INVENTORY) {
		// Přidáme na konec (vrchol zásobníku)
		player_inventory[inventory_count] = type;
		inventory_count++;
		
		// Překreslíme levý panel (např. pozice X=2, Y=80, 90, 100)
		// Y pozice se počítá dynamicky: 80 + index * 10
		uint8_t draw_y = 80 + ((inventory_count - 1) * 10);
		draw_powerup8x8(2, draw_y, type, COLOR_BG);
		
		// Zvuk sebrání
		// Soundboard::playSound(Soundboard::sfx_powerup_get);
		} else {
		// Inventář je plný, power-up je zničen bez užitku
		// Možná přehrát nějaký chybový zvuk
	}
}

/************************* spawn_sentry_gun *************************/
void spawn_sentry_gun(uint8_t ship_x) {
	for (int i = 0; i < MAX_SENTRIES; i++) {
		if (!active_sentries[i].active) {
			active_sentries[i].active = true;
			active_sentries[i].x = ship_x;
			
			// Položíme ji například 5 pixelů nad loď
			active_sentries[i].y = SHIP_Y - 5;
			
			// Zaznamenáme si aktuální čas pro životnost i střelbu
			// (použij funkci, kterou tvůj framework používá pro zjištění mikrosekund, např. micros())
			uint32_t current_time = micros();
			active_sentries[i].spawn_time = current_time;
			active_sentries[i].last_shot_time = current_time;
			
			break; // Položeno, končíme hledání volného místa
		}
	}
}

/************************* use_powerup *************************/
// Funkce pro odebrání power-up ze zásobníku
void use_powerup() {
	if (inventory_count > 0) {
		// Získáme ten naposledy přidaný (index je o 1 menší než počet)
		inventory_count--;
		PowerUpType used_type = player_inventory[inventory_count];
		
		// Smažeme ho z displeje na levém panelu
		uint8_t erase_y = 80 + (inventory_count * 10);
		st7735_fill_rect(2, erase_y, 8, 8, COLOR_BG);
		
		// Aktivace a (re)startování specifického timeru
		switch(used_type) {
			case PU_RAPID_FIRE:
				isRapidFireActive = true;
				rapidFirePUTimer->startTimerUs(LIFETIME_RAPID);
				break;
			
			case PU_SHIELD:
				isShieldActive = true;
				shieldPUTimer->startTimerUs(LIFETIME_SHIELD);
				// Vykreslení lodi jinou barvou se vyřeší rovnou v draw_ship
				break;
			
			case PU_SENTRY:
				// Sentry se položí na mapu
				spawn_sentry_gun(x);
				break;
		}
		Soundboard::playSound(Soundboard::sfx_powerup_use);
	}
}

/************************* spawn_random_powerup *************************/
void spawn_random_powerup() {
	for (int i = 0; i < MAX_POWERUPS; i++) {
		if (!active_powerups[i].active) {
			active_powerups[i].active = true;
			
			// X pozice: od 10 do 110 (aby nebyl power-up nalepený úplně na okrajích displeje)
			active_powerups[i].x = 10 + (rand() % 100);
			
			// Y pozice: horní polovina obrazovky (např. od Y=20 do Y=90)
			active_powerups[i].y = 20 + (rand() % 70);
			
			// Náhodný typ (0, 1, nebo 2 - odpovídá našemu enum PowerUpType)
			active_powerups[i].type = (PowerUpType)(rand() % 3);
			
			active_powerups[i].health = 11; 
			active_powerups[i].spawn_time = micros(); // Zaznamenáme čas spawnu
			
			// Vykreslení power-upu
			draw_powerup8x8(active_powerups[i].x, active_powerups[i].y, active_powerups[i].type, COLOR_BG);
			
			break; // Místo nalezeno, víc jich teď nespawnujeme
		}
	}
}

/************************* update_hud_ammo *************************/
// Update počtu nábojů po střelbě
void update_hud_ammo(WeaponType wep, uint8_t new_ammo) {
	// Railgun nekonečno neaktualizujeme
	if (wep == WEP_RAILGUN) return;

	// Výpočet pozice zbraně v HUDu (stejný vzorec jako při initu)
	uint8_t slot_x = 43 + static_cast<int>(wep) * 21;
	
	// Pozice textu s náboji (vycházíme z gameplay_draw_bottom_hud)
	uint8_t text_x = slot_x + 5;
	uint8_t text_y = BOT_HUD_Y_START + 11;

	// 1. Smažeme staré číslo.
	// Předpokládám standardní font 5x7. Dvě číslice vedle sebe zaberou na šířku cca 11-12 pixelů a 7 na výšku.
	// Pro jistotu smažeme obdélník 12x8 pixelů.
	st7735_fill_rect(text_x, text_y, 12, 8, COLOR_BG);

	// 2. Naformátujeme a vykreslíme nové číslo
	char ammo_s[3];
	ammo_s[0] = (new_ammo / 10) + '0';
	ammo_s[1] = (new_ammo % 10) + '0';
	ammo_s[2] = '\0';
	
	draw_string(text_x, text_y, ammo_s, COLOR_WHITE, COLOR_BG);
}

/************************* process_projectile *************************/
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
		// Paprsek se v ose Y neposouvá, jen trvá 1.5 sekundy
		if (micros() - p.spawn_time > 1500000UL) {
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

/************************* spawn_projectile *************************/
// Pomocná funkce pro vytvoření jedné fyzické střely
void spawn_projectile(WeaponType wep, uint8_t ship_x, uint8_t ship_y) {
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		if (!player_lasers[i].active) {
			player_lasers[i].active = true;
			player_lasers[i].x = ship_x + 7; // Vycentrování
			player_lasers[i].y = (wep == WEP_ROCKET) ? (ship_y - 8) : (ship_y - 1);
			
			// Burst reálně střílí malé railgun projektily
			player_lasers[i].type = (wep == WEP_BURST) ? WEP_RAILGUN : wep;
			player_lasers[i].spawn_time = micros(); // Pro laser

			// Zvuky přesunuty sem
			if (wep == WEP_RAILGUN) Soundboard::playSound(Soundboard::sfx_railgun);
			if (wep == WEP_ROCKET) Soundboard::playSound(Soundboard::sfx_rocket);
			if (wep == WEP_LASER) Soundboard::playSound(Soundboard::sfx_laser);
			if (wep == WEP_BURST) Soundboard::playSound(Soundboard::sfx_burst);

			break; // Vytvořeno, končíme hledání
		}
	}
}

/************************* add_ammo *************************/
// Přičtení nábojů po zvednutí power-up
void add_ammo(WeaponType wep, uint8_t amount) {
	// Přičteme náboje
	player_ammo[wep] += amount;
	
	// Omezíme na max 99 kvůli vykreslování na displeji
	if (player_ammo[wep] > 99) {
		player_ammo[wep] = 99;
	}
}

/************************* try_shoot *************************/
// Funkce, která řeší, ZDA vůbec můžeme vystřelit
void try_shoot(WeaponType wep, uint8_t ship_x, uint8_t ship_y) {
	// Zjistíme, jaký má hráč momentálně limit střel
	uint8_t allowed_shots = isRapidFireActive ? 12 : 6;
	
	// Spočítáme, kolik střel má hráč aktuálně na obrazovce
	uint8_t active_count = 0;
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		if (player_lasers[i].active) {
			active_count++;
		}
	}
	
	// Pokud jsme přesáhli limit, zablokujeme výstřel (přehraj cvaknutí naprázdno)
	if (active_count >= allowed_shots) {
		Soundboard::playSound(Soundboard::sfx_noAmmo);
		return;
	}
	// Kontrola nábojů
	if (wep == WEP_RAILGUN || player_ammo[wep] > 0) {
		
		// Zahájení střelby
		if (wep == WEP_BURST) {
			// První střela vyletí IHNED, nečekáme na timer
			spawn_projectile(WEP_BURST, ship_x, ship_y);
			burst_shots_left = 2; // Zbývají už jen 2
			burstTimer->startTimerUs(300000); 
			} else {
			spawn_projectile(wep, ship_x, ship_y);
		}

		// Odečtení náboje a update HUDu
		if (wep != WEP_RAILGUN) {
			player_ammo[wep]--;
			update_hud_ammo(wep, player_ammo[wep]);
		}
		} else {
		// Hráč nemá náboje
		Soundboard::playSound(Soundboard::sfx_noAmmo);
	}
}

/************************* spawn_enemy_projectile *************************/
void spawn_enemy_projectile(uint8_t received_x, WeaponType wep_type) {
	// 127 je šířka obrazovky. Tímto získáme přesně zrcadlovou pozici.
	uint8_t mirrored_x = 127 - received_x;
	
	for (int i = 0; i < (MAX_PROJECTILES); i++) {
		// Hledáme volné místo ve vyhrazeném poli pro NEPŘÁTELE
		if (!enemy_lasers[i].active) {
			enemy_lasers[i].active = true;
			enemy_lasers[i].x = mirrored_x;
			enemy_lasers[i].y = TOP_HUD_Y_END + 1; // Začíná nahoře pod radarem
			enemy_lasers[i].type = wep_type; // Zbraň podle parametru
			
			// Pokud stál časovač střel (na obrazovce nic neletělo), spustíme ho
			if (projectileTimer->isDone()) {
				projectileTimer->startTimerUs(25000);
			}
			break; 
		}
	}
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

/************************* gameplay_init *************************/
// Inicializace herního módu
void gameplay_init(void) {
	// Alokujeme timery POUZE pokud ještě nebyly alokovány
	if (btnTimer == nullptr) {
		btnTimer = SoftwareTimerPool::acquireTimer();
		projectileTimer = SoftwareTimerPool::acquireTimer();
		burstTimer = SoftwareTimerPool::acquireTimer();
		fireCooldownTimer = SoftwareTimerPool::acquireTimer();
		holdTimer = SoftwareTimerPool::acquireTimer();
		rapidFirePUTimer = SoftwareTimerPool::acquireTimer();
		shieldPUTimer = SoftwareTimerPool::acquireTimer();
		powerupSpawnTimer = SoftwareTimerPool::acquireTimer();
	}
	
	burstTimer->startTimerUs(100);
	fireCooldownTimer->startTimerUs(0);
	powerupSpawnTimer->startTimerUs(5000000);                                           // Později změnit na 20 sec
    
    last_A = (PIND & (1 << PD2)) >> PD2;
    x = DISPLAY_WIDTH/2; // Startovní pozice lodě
	btn_state = BTN_IDLE;	
	current_weapon = WEP_RAILGUN; // Aktuálně zvolená zbraň
	burst_shots_left = 0;
	inventory_count = 0;
	isShieldActive = false;
	isRapidFireActive = false;
    
	// Promazání paměti objektů
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        player_lasers[i].active = false;
        enemy_lasers[i].active = false;
    }
	for (int i = 0; i < MAX_POWERUPS; i++) {
		active_powerups[i].active = false;
	}
	for (int i = 0; i < MAX_SENTRIES; i++) {
		active_sentries[i].active = false;
	}
    
    st7735_fill_screen(COLOR_BG);
    gameplay_draw_top_hud_static();
    gameplay_draw_top_hud_dynamic(99, 99); // P1_Hp, P2_Hp
    gameplay_draw_bottom_hud(99, WEP_RAILGUN, player_ammo); // your_hp, wep, ammo_array
    draw_ship(x, SHIP_Y, COLOR_BLUE);
}

/************************* gameplay_tick *************************/
// Herní logika
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
				if (!(PIND & (1 << PD5))) { // Stisknuto tlačítko střelby
			    
					try_shoot(current_weapon, x, SHIP_Y);
			    
					// Timer sdílí všechny střely, takže ho pustíme, pokud ještě neběží
					projectileTimer->startTimerUs(80000);
			    
					// Nastartování cooldownu podle toho, jestli máme Power-up
					uint32_t current_cd = isRapidFireActive ? COOLDOWNS_RAPID[current_weapon] : COOLDOWNS_NORMAL[current_weapon];
					fireCooldownTimer->startTimerUs(current_cd);
					
					// Nastartujeme timer pro detekci dlouhého stisku
					holdTimer->startTimerUs(POWERUP_HOLD_TIME);
			    
					btn_state = BTN_WAIT_RELEASE;
					} else {
						btn_state = BTN_IDLE; // Falešný stisk (tlačítko bylo puštěno moc brzo)
					}
			}
			break;

	    case BTN_WAIT_RELEASE:
	    if (PIND & (1 << PD5)) {
		    // Tlačítko bylo puštěno PŘED vypršením hold timeru -> normální stisk
		    btnTimer->startTimerUs(50000);
		    btn_state = BTN_RELEASE_DEBOUNCE;
	    }
	    else if (holdTimer->isDone()) {
		    // Tlačítko je STÁLE stisknuté a časovač vypršel -> DLOUHÝ STISK!
		    use_powerup();
		    
		    // Přepneme do nového stavu, kde jen čekáme, až ho hráč pustí
		    btn_state = BTN_WAIT_FULL_RELEASE;
	    }
	    break;

	    case BTN_WAIT_FULL_RELEASE:
	    if (PIND & (1 << PD5)) {
		    // Hráč konečně pustil tlačítko po aktivaci power-upu
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

    // --- LOGIKA PRO DOKONČENÍ BURST DÁVKY ---
    if (burst_shots_left > 0 && burstTimer->isDone()) {
		spawn_projectile(WEP_BURST, x, SHIP_Y);
		burst_shots_left--;

		if (burst_shots_left > 0) {
			burstTimer->startTimerUs(250000);
		}
	}

    // --- FYZIKA A VYKRESLOVÁNÍ PROJEKTILŮ ---
    if (projectileTimer->isDone()) {
	    bool any_laser_active = false;

		// A) HRÁČOVY STŘELY
		for (int i = 0; i < MAX_PROJECTILES; i++) {
			if (player_lasers[i].active) {
				any_laser_active = true;
				process_projectile(player_lasers[i], false); // false = není to nepřítel
			}
		}

		// B) NEPŘÁTELSKÉ STŘELY
		for (int i = 0; i < MAX_PROJECTILES; i++) {
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
	
	uint32_t current_time = micros();

	// --- UPDATE SENTRY GUNŮ ---
	for (int i = 0; i < MAX_SENTRIES; i++) {
		if (active_sentries[i].active) {
					
			// 1. Kontrola, jestli věži nevypršel čas
			if (current_time - active_sentries[i].spawn_time > LIFETIME_SENTRY) {
				active_sentries[i].active = false;
				// Zde můžeš překreslit pozadí (smazat věž z obrazovky)
				st7735_fill_rect(active_sentries[i].x, active_sentries[i].y, 8, 8, COLOR_BG);
				continue; // Přeskočíme zbytek smyčky pro tuto zničenou věž
			}
					
			// 2. Kontrola střelby
			if (current_time - active_sentries[i].last_shot_time > COOLDOWN_SENTRY) {
				// Věž střílí (jako typ pošleme Railgun z její pozice)
				// Pokud máš spawn_projectile navržený jen pro hráče, normálně to použij
				spawn_projectile(WEP_RAILGUN, active_sentries[i].x, active_sentries[i].y);
						
				// Resetujeme časovač střelby
				active_sentries[i].last_shot_time = current_time;
			}
					
			// 3. Vykreslení věže (tuto funkci zavoláš s tvou novou bitmapou)
			// draw_bitmap_PROGMEM(active_sentries[i].x, active_sentries[i].y, 8, 8, sentry_gun_bitmap, COLOR_SENTRY, COLOR_BG);
		}
	}
	
	// --- SPAWNOVÁNÍ NOVÝCH POWER-UPŮ ---
	if (powerupSpawnTimer->isDone()) {
		spawn_random_powerup();
		
		// Nastavíme timer na další náhodný čas (např. za 4 až 20 sekund)
		// random() u velkých čísel může někdy na 8bitu zlobit, proto to napíšeme bezpečně:
		uint32_t random_delay = 4000000 + (rand()%20 * 1000UL * 1000UL); // 4s + 0 až 20s
		powerupSpawnTimer->startTimerUs(random_delay);
	}

	// --- UPDATE EXISTUJÍCÍCH POWER-UPŮ ---
	//uint32_t current_time = micros();
	current_time = micros();
	for (int i = 0; i < MAX_POWERUPS; i++) {
		if (active_powerups[i].active) {
			
			// Power-up leží na ploše např. 6 sekund. Pokud ho hráč nesestřelí, zmizí.
			if (current_time - active_powerups[i].spawn_time > 6000000) {
				active_powerups[i].active = false;
				// Smazání ikony z hrací plochy (překreslení barvou pozadí)
				st7735_fill_rect(active_powerups[i].x, active_powerups[i].y, 8, 8, COLOR_BG);
				continue;
			}
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
				uint16_t ship_color = isShieldActive ? COLOR_WHITE : COLOR_BLUE; 
			    draw_ship(x, SHIP_Y, ship_color);
		    }
	    }
	    last_A = current_A;
    }	
	
	// --- KONTROLA VYPRŠENÍ POWER-UPŮ ---
	if (isRapidFireActive && rapidFirePUTimer->isDone()) {
		isRapidFireActive = false;
	}
	
	if (isShieldActive && shieldPUTimer->isDone()) {
		isShieldActive = false;
		draw_ship(x, SHIP_Y, COLOR_BLUE);
		// Volitelně přehraj zvuk spadnutí štítu
	}
	
	return STATE_GAMEPLAY;
}