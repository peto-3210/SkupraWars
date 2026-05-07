#include "Gameplay.hpp"
#include "Graphics/Graphics.hpp"
#include "Gamestate.hpp"
#include "Utilities/Soundboard.hpp"
#include "Utilities/SoftwareTimer.hpp"
#include "hal/st7735.h"
#include "hal/input.h"
#include "hal/libraries.h"
#include <stdio.h>
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

#define MAX_PROJECTILES 17
#define RAILGUN_LENGTH 5
#define SHIP_LENGTH 16

// Řešení power ups
#define MAX_INVENTORY 3
#define MAX_POWERUPS 4
#define POWERUP_HOLD_TIME 666000  // 666 ms pro dlouhý stisk
#define LIFETIME_RAPID 5000000    // 5 sec
#define LIFETIME_SHIELD 6000000   // 6.0 sec
#define LIFETIME_SENTRY 9000000  // 9.0 sec
#define COOLDOWN_SENTRY 2000000   // 2.0 sec

// Časy jsou v mikrosekundách
const uint32_t COOLDOWNS_NORMAL[4] = {
	700000,  // WEP_RAILGUN  (0.7 sec)
	900000,  // WEP_BURST    (0.9 sec)
	1200000, // WEP_ROCKET   (1.2 sec)
	1000000   // WEP_LASER   (1.0 sec)
};

const uint32_t COOLDOWNS_RAPID[4] = {
	450000,  // WEP_RAILGUN  (0.45 sec)
	600000,  // WEP_BURST    (0.6 sec)
	900000,  // WEP_ROCKET   (0.9 sec)
	700000   // WEP_LASER    (0.7 sec)
};

// Globální stav nábojů hráče
// Indexy odpovídají: 0=RAILGUN, 1=BURST, 2=ROCKET, 3=LASER
uint8_t player_ammo[4] = {
	255,  // WEP_RAILGUN: Hodnota je fuk, HUD kreslí nekonečno a v logice ho nebudeme odečítat
	10,   // WEP_BURST: Startovní počet nábojů
	10,   // WEP_ROCKET: Startovní počet raket
	10    // WEP_LASER: Startovní počet laserů
};

// --- Lokální proměnné pro hru ---
static bool is_rng_seeded = false;
bool isShieldActive;
bool isRapidFireActive; 
WeaponType current_weapon;
static ButtonState btn_state;
static bool last_enc_btn = false; // Pro detekci hrany enkodérového tlačítka
uint8_t inventory_count;
static uint8_t x; // Pozice lodě
static const uint8_t SHIP_Y = DISPLAY_LENGTH - 36;
uint8_t burst_shots_left;
static uint8_t last_A;
static uint8_t player_hp = 0;
static uint8_t enemy_hp[2] = {0, 0}; // [0] je levý, [1] je pravý


PowerUpType player_inventory[MAX_INVENTORY];
SentryGun active_sentries[MAX_SENTRIES];
PowerUp active_powerups[MAX_POWERUPS];
static Projectile player_lasers[MAX_PROJECTILES];
static Projectile enemy_lasers[MAX_PROJECTILES];

// Alokace timerů
static SoftwareTimer* btnTimer = nullptr;
static SoftwareTimer* projectileTimer = nullptr;
static SoftwareTimer* fireCooldownTimer = nullptr;
static SoftwareTimer* holdTimer = nullptr;
static SoftwareTimer* rapidFirePUTimer = nullptr;
static SoftwareTimer* shieldPUTimer = nullptr;
static SoftwareTimer* powerupSpawnTimer = nullptr;
static SoftwareTimer* burstTimer = nullptr; 

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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
	
	draw_char_buffer(text_x, text_y, ammo_s, 0, COLOR_WHITE, COLOR_BG);
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

	// Okamžitá aktualizace čísla na displeji
    update_hud_ammo(wep, player_ammo[wep]);
}

/************************* add_powerup_to_inventory *************************/
// Funkce pro přidání power-up do zásobníku a zisk náhodné munice
void add_powerup_to_inventory(PowerUpType type) {
	if (inventory_count < MAX_INVENTORY) {
		// Přidáme na konec (vrchol zásobníku)
		player_inventory[inventory_count] = type;
		inventory_count++;
		
		// Překreslíme levý panel
		uint8_t draw_y = 100 - ((inventory_count - 1) * 10);
		draw_powerup8x8(2, draw_y, type, COLOR_BG);

		update_inventory_ui(inventory_count, player_inventory);
		
		// Zvuk sebrání power-upu
		// Soundboard::playSound(Soundboard::sfx_powerup_get);
	} else {
		// Inventář je plný, power-up je zničen (ikona se nepřidá)
		// Možná přehrát nějaký chybový zvuk
	}

    // --- Náhodná munice při KAŽDÉM sebrání ---
    
    // Náhodná zbraň (1 až 3). Vyhneme se nule (WEP_RAILGUN).
    WeaponType random_wep = static_cast<WeaponType>((rand() % 3) + 1);
    
    // Náhodný počet nábojů (např. od 3 do 8 kusů)
    uint8_t random_amount = 3 + (rand() % 5);
    
    // Přidáme hráči náboje (funkce se sama postará o update HUDu)
    add_ammo(random_wep, random_amount);
}

/************************* spawn_sentry_gun *************************/
void spawn_sentry_gun(uint8_t ship_x) {
	for (int i = 0; i < MAX_SENTRIES; i++) {
		if (!active_sentries[i].active) {
			active_sentries[i].active = true;
			active_sentries[i].x = ship_x;
			
			// Položíme ji například 10 pixelů nad loď
			active_sentries[i].y = SHIP_Y - 10;
			
			// Zaznamenáme si aktuální čas pro životnost i střelbu
			uint32_t current_time = micros();
			active_sentries[i].spawn_time = current_time;
			active_sentries[i].last_shot_time = current_time;
			
			break; 
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
		uint8_t erase_y = 100 - (inventory_count * 10);
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
				draw_ship(x, SHIP_Y, COLOR_WHITE); // Loď bílou -> power-up je aktivní
				break;
			
			case PU_SENTRY:
				// Sentry se položí na mapu
				spawn_sentry_gun(x);
				break;
		}
		update_inventory_ui(inventory_count, player_inventory);
		Soundboard::playSound(Soundboard::sfx_powerup_use);
	}
}

/************************* powerup_spawn_random *************************/
bool powerup_spawn_random() {
	for (int i = 0; i < MAX_POWERUPS; i++) {
		if (!active_powerups[i].active) {
			active_powerups[i].active = true;
			
			// X pozice: od 14 do 114 (aby nebyl power-up nalepený úplně na okrajích displeje)
			active_powerups[i].x = 14 + (rand() % 100);
			
			// Y pozice: horní polovina obrazovky (např. od Y=20 do Y=100)
			active_powerups[i].y = 20 + (rand() % 80);
			
			// Náhodný typ (odpovídá enum PowerUpType)
			active_powerups[i].type = (PowerUpType)(rand() % 3);
			
			active_powerups[i].health = 11; 
			active_powerups[i].spawn_time = micros(); // Zaznamenáme čas spawnu
			
			// Vykreslení power-upu
			draw_powerup8x8(active_powerups[i].x, active_powerups[i].y, active_powerups[i].type, COLOR_BG);
			
			return true;
		}
	}
	return false;
}

/************************* get_weapon_damage *************************/
// Zjištění poškození zbraně
uint8_t get_weapon_damage(WeaponType wep) {
	switch(wep) {
		case WEP_RAILGUN: return 4;  // Powerup má 11 HP -> zničí se na 3 rány
		case WEP_BURST:   return 2;  // Burst střílí 3 projektily, celkem dá 6 DMG
		case WEP_ROCKET:  return 11; 
		case WEP_LASER:   return 1;  
		default: return 1;
	}
}

/************************* check_collision *************************/
// AABB Kolize (Vrátí true, pokud se dva obdélníky překrývají)
bool check_collision(int x1, int y1, int w1, int h1,
					 int x2, int y2, int w2, int h2) {
	return (x1 < x2 + w2 && x1 + w1 > x2 &&
			y1 < y2 + h2 && y1 + h1 > y2);
}

/************************* check_powerup_collisions *************************/
// Vrátí 'true', pokud byl projektil pohlcen power-upem a má se zastavit
bool check_powerup_collisions(Projectile& p) {
	int px = p.x, py = p.y, pw = 1, ph = 1;
	
	// A) Určení "hitboxu" podle typu zbraně
	if (p.type == WEP_ROCKET) {
		pw = 3; ph = 8;
	} else if (p.type == WEP_LASER) {
		px = p.x - 1; pw = 2; // Laser lodi je uprostřed
		py = 16; ph = SHIP_Y - 16; // Přes celou obrazovku
	} else {
		// Railgun / Burst (Špička je na p.y, ohon se táhne dolů o RAILGUN_LENGTH)
		pw = 1; ph = RAILGUN_LENGTH;
		py = p.y;
	}

	// B) Průchod všemi power-upy
	for (int i = 0; i < MAX_POWERUPS; i++) {
		if (active_powerups[i].active) {
			
			// Power-up má rozměry 8x8 pixelů
			if (check_collision(px, py, pw, ph, active_powerups[i].x, active_powerups[i].y, 8, 8)) {
				
				// ZÁSAH!
				uint8_t dmg = get_weapon_damage(p.type);
				
				if (active_powerups[i].health <= dmg) {
					// Power-up byl zničen -> Přidání hráči
					active_powerups[i].active = false;
					st7735_fill_rect(active_powerups[i].x, active_powerups[i].y, 8, 8, COLOR_BG);
					add_powerup_to_inventory(active_powerups[i].type);

				} else {
					// Power-up jen dostal poškození
					active_powerups[i].health -= dmg;
					// Překreslíme ho, aby ho projektil vizuálně "neprořízl"
					draw_powerup8x8(active_powerups[i].x, active_powerups[i].y, active_powerups[i].type, COLOR_BG);
				}

				// C) Pohlcení projektilu (laser nepolykáme, ten je trvalý)
				if (p.type != WEP_LASER) {
					p.active = false;
					
					// Fyzické smazání projektilu z displeje, ať neletí dál jako duch
					if (p.type == WEP_ROCKET) {
						st7735_fill_rect(p.x, p.y, 3, 9, COLOR_BG);
					} else {
						for(uint8_t j = 0; j <= RAILGUN_LENGTH; j++) {
							st7735_draw_pixel(p.x, p.y + j, COLOR_BG);
						}
					}
					return true; // Projektil je zničen!
				}
			}
		}
	}
	return false; // Projektil nic netrefil, letí dál
}

/************************* update_player_hp_ui *************************/
void update_player_hp_ui(uint8_t hp) {
    char hp_str[4];
    // "%02d" zaručí, že číslo bude mít vždy 2 znaky (např. "05", "99")
    sprintf(hp_str, "%02d", hp); 

    // Vykreslíme text "hp_str". 
    // Délka je 2 (protože máme 2 číslice).
    draw_char_buffer(18, BOT_HUD_Y_START + 6, hp_str, 2, COLOR_RED, COLOR_BG);
}

/************************* update_enemy_hp_ui *************************/
void update_enemy_hp_ui(uint8_t enemy_index, uint8_t hp) {
    char hp_str[4];
    sprintf(hp_str, "%02d", hp);

    // Výpočet souřadnice X podle indexu nepřítele:
    // Pokud je to levý nepřítel (0), X bude 18 (levý okraj).
    // Pokud je to pravý nepřítel (1), X bude např. 115 (pravý okraj).
    // (Tyto hodnoty 4 a 110 si uprav podle toho, kam to přesně chceš nakreslit)
    int x_pos = (enemy_index == 0) ? 18 : 100;
	uint16_t color = (enemy_index == 0) ? COLOR_GREEN : COLOR_BLUE;

    draw_char_buffer(x_pos, TOP_HUD_Y_START + 4, hp_str, 2, color, COLOR_BG);
}

/************************* check_player_collision *************************/
// Vrátí true, pokud nepřátelská střela trefila naši loď
bool check_player_collision(Projectile& p) {
	int px = p.x, py = p.y, pw = 1, ph = 1;
	
	// Stejný hitbox jako u power-upů
	if (p.type == WEP_ROCKET) {
		pw = 3; ph = 8;
	} else if (p.type == WEP_LASER) {
		px = p.x - 1; pw = 2;
		py = 16; ph = SHIP_Y;
	} else {
		pw = 1; ph = RAILGUN_LENGTH;
		py = p.y;
	}

    // Štít: Pokud je štít aktivní, loď je nezranitelná...
	if (check_collision(px, py, pw, ph, x, SHIP_Y, SHIP_LENGTH, SHIP_LENGTH)) {
		
		// ZÁSAH DO LODĚ BEZ ŠTÍTU!
		if (!isShieldActive) {
			
			// --- TICK TIMER (I-FRAMES) ---
			// Tato proměnná se vytvoří jen jednou a pamatuje si čas napořád
			static uint32_t last_damage_time = 0;
			
			// Může hráč dostat poškození? (Základně ano)
			bool can_take_damage = true;
			
			// Pokud je to laser, zkontrolujeme, jestli od posledního zranění uběhlo alespoň 100 ms
			if (p.type == WEP_LASER) {
				if (micros() - last_damage_time < 70000UL) {
					can_take_damage = false; // Ještě neuběhlo půl vteřiny, poškození ignorujeme
				}
			}
			
			// Pokud je poškození povoleno, odečteme životy a zresetujeme časovač
			if (can_take_damage) {
				last_damage_time = micros(); // Uložíme čas tohoto zásahu
				
				uint8_t dmg = get_weapon_damage(p.type);          
				// Ošetření podtečení HP
				if (player_hp > dmg) {
					player_hp -= dmg;
				} else {
					player_hp = 0; 
					// ZDE BUDE KONEC HRY (SMRT)
				}
				// Překreslíme náš dolní panel
				update_player_hp_ui(player_hp);				
				// Přehrajeme zvuk zásahu
				Soundboard::playSound(Soundboard::sfx_hit_enemy);
			}
		}

		// --- Odeslání mého nového HP přes UART ---
		// uart_send_hp(player_hp); 
        

		// Pohlcení projektilu (stejně jako u powerupů, laser nepolykáme)
		if (p.type != WEP_LASER) {
			p.active = false;
			if (p.type == WEP_ROCKET) {
				st7735_fill_rect(p.x, p.y, 3, 9, COLOR_BG);
			} else {
				for(uint8_t j = 0; j <= RAILGUN_LENGTH; j++) {
					st7735_draw_pixel(p.x, p.y - j, COLOR_BG); // POZOR: Nepřítel má ocas nahoře (-j)
				}
			}
			return true;
		}
	}
	return false;
}

/************************* on_enemy_hp_received *************************/
void apply_enemy_hp_update(uint8_t enemy_index, uint8_t new_hp) {
    // enemy_index: 0 = levý, 1 = pravý
    // Uložíme nové HP správnému nepříteli do našeho pole
    enemy_hp[enemy_index] = new_hp;
    
    // Překreslíme ho na displeji
    update_enemy_hp_ui(enemy_index, new_hp);
    
    if (enemy_hp[enemy_index] == 0) {
        // ZDE BUDE KÓD PRO SMRT DANÉHO NEPŘÍTELE
        // Např. mu tam můžeš nakreslit explozi nebo "00" a zneaktivnit jeho stranu
    }
}

/************************* safe_draw_pixel *************************/
// Bezpečné vykreslení pixelu (pokud padne do inventáře, zahodí se)
void safe_draw_pixel(int x, int y, uint16_t color) {
    // Pokud jsme v inventáři, nekreslíme (okamžitě ukončíme funkci)
    if (x <= 13 && y >= 77 && y <= 111) {
        return; 
    }
    st7735_draw_pixel(x, y, color);
}

/************************* safe_fill_rect *************************/
// Bezpečné vykreslení obdélníku (pro rakety a kontinuální lasery)
void safe_fill_rect(int x, int y, int w, int h, uint16_t color) {
    // Pokud je obdélník mimo zakázanou X zónu, vykreslíme ho rovnou celý (rychlé)
    if (x > 13) {
        st7735_fill_rect(x, y, w, h, color);
        return;
    }
    
    // Pokud jsme v kolizní X zóně, musíme obdélník kreslit po jednotlivých řádcích
    // Tím zajistíme, že raketa "zajede" pod inventář postupně a plynule
    for (int i = 0; i < h; i++) {
        int current_y = y + i;
        
        // Přeskočíme řádky, které spadají do inventáře
        if (current_y >= 77 && current_y <= 111) {
            continue; 
        }
        
        // Vykreslíme jen ten jeden povolený řádek obdélníku
        st7735_fill_rect(x, current_y, w, 1, color);
    }
}

/************************* redraw_powerups_under_tail *************************/
// Překreslí power-upy, které byly graficky smazány "úklidem" po nepřátelské střele
void redraw_powerups_under_tail(int erase_x, int erase_y, int erase_w, int erase_h) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (active_powerups[i].active) {
            // Jednoduchá matematika pro protnutí dvou obdélníků
            // Pokud černý flek zasahuje do 8x8 plochy power-upu...
            if (erase_x < active_powerups[i].x + 8 && erase_x + erase_w > active_powerups[i].x &&
                erase_y < active_powerups[i].y + 8 && erase_y + erase_h > active_powerups[i].y) {
                
                // ...tak ho okamžitě překreslíme zpět!
                draw_powerup8x8(active_powerups[i].x, active_powerups[i].y, active_powerups[i].type, COLOR_BG);
            }
        }
    }
}

/************************* process_projectile *************************/
// Funkce přijme referenci na JAKOUKOLIV střelu a informaci, čí ta střela je
void process_projectile(Projectile& p, bool is_enemy) {
	
	// Zjistíme směr a hranici zániku podle toho, kdo střílí
	int direction = is_enemy ? 1 : -1;
	
	// --- 1. RAILGUN & BURST ---
	if (p.type == WEP_RAILGUN || p.type == WEP_BURST) {
		
		// Zjištění pozice ocasu střely
		int tail_offset = is_enemy ? -RAILGUN_LENGTH : RAILGUN_LENGTH;
		int tail_y = p.y + tail_offset;
		
		// Smažeme ocas střely, pokud je ještě na obrazovce
		bool tail_on_screen = is_enemy ? (tail_y >= 16) : (tail_y < SHIP_Y);
		if (tail_on_screen) {
			safe_draw_pixel(p.x, tail_y, COLOR_BG);
			if (is_enemy) redraw_powerups_under_tail(p.x, tail_y, 1, 1);
		}
		
		// Barvy podle toho, kdo střílí (Hráč: modrá stopa/světle modrá špička | Nepřítel: červená stopa/žlutá špička)
		uint16_t trace_color = is_enemy ? COLOR_RED : COLOR_BLUE;
		uint16_t tip_color   = is_enemy ? COLOR_ORANGE : COLOR_CYAN;

		// Stará špička ztmavne
		safe_draw_pixel(p.x, p.y, trace_color);
		
		// Posun
		p.y += direction;

		// Kontrola kolize s power-upy (pouze pro hráče!)
        if (!is_enemy && check_powerup_collisions(p)) { return; }
		// Kontrola kolize nepřátelských střel s naší lodí
		if (is_enemy && check_player_collision(p)) { return; }
		
		// Nová zářivá špička
		safe_draw_pixel(p.x, p.y, tip_color);

		// Kontrola okrajů
		if (is_enemy) {
            // Špička nepřátelské střely (p.y) narazí na červenou čáru HUDu
            if (p.y >= BOT_SEP_LINE_Y) {
                p.active = false;
                for(uint8_t j = 0; j <= RAILGUN_LENGTH; j++) {
                    safe_draw_pixel(p.x, p.y - j, COLOR_BG);
                    redraw_powerups_under_tail(p.x, p.y - j, 1, 1);
                }
                // Jistota - obnovíme ten jeden pixel červené čáry
                safe_draw_pixel(p.x, BOT_SEP_LINE_Y, COLOR_RED);
            }
        } else {
            if (p.y <= 16) {
                p.active = false;
                for(uint8_t j = 0; j <= RAILGUN_LENGTH; j++) {
                    safe_draw_pixel(p.x, p.y + j, COLOR_BG);
                }
            }
        }
    }
	
	// --- 2. RAKETOMET ---
	else if (p.type == WEP_ROCKET) {
		
		// Smazání stopy po raketě (spodní/horní řádek, který po posunu zůstane)
		int erase_y = is_enemy ? (p.y) : (p.y + 7);
		if (erase_y >= 16 && (is_enemy || erase_y < SHIP_Y)) {
            safe_fill_rect(p.x, erase_y, 3, 1, COLOR_BG);
            if (is_enemy) redraw_powerups_under_tail(p.x, erase_y, 3, 1);
        }

		// Posun
		p.y += direction;

		// Kontrola kolize
        if (!is_enemy && check_powerup_collisions(p)) { return; }
        if (is_enemy && check_player_collision(p)) { return; }

		// Rozlišení barev a umístění křidélek (aby raketa letěla špičkou dopředu)
		uint16_t body_color = is_enemy ? COLOR_RED : COLOR_GREEN; // Hráč zelená, nepřítel červená
		uint16_t wing_color = is_enemy ? COLOR_BLUE : COLOR_ORANGE; // Hráč oranžová, nepřítel modrá
		int wing_y_offset   = is_enemy ? 0 : 4;           // Křidélka vzadu

		// Vykreslení rakety na nové pozici (x, y, šířka, výška)
		safe_fill_rect(p.x + 1, p.y, 1, 8, body_color);                    // Tělo
		safe_fill_rect(p.x, p.y + wing_y_offset, 1, 4, wing_color);        // Levé křidélko
		safe_fill_rect(p.x + 2, p.y + wing_y_offset, 1, 4, wing_color);    // Pravé křidélko

		// Kontrola okrajů
		if (is_enemy) {
            // p.y je u rakety HORNÍ roh. Spodní hrana je p.y + 8.
            if ((p.y + 8) >= BOT_SEP_LINE_Y) {
                p.active = false;
                // Smažeme raketu
                safe_fill_rect(p.x, p.y, 3, 8, COLOR_BG);
                redraw_powerups_under_tail(p.x, p.y, 3, 8);
                
                // Obnovíme kus červené čáry, pokud jsme ho smazali
                //safe_fill_rect(p.x, BOT_SEP_LINE_Y, 3, 1, COLOR_RED);
            }
        } else {
            if (p.y <= 16) {
                p.active = false;
                // Hráčova raketa letí nahoru, ocas je pod ní, mažeme prostor dolů
                safe_fill_rect(p.x, p.y, 3, 8, COLOR_BG);
            }
        }
    }
	
	// --- 3. LASER ---
    else if (p.type == WEP_LASER) {
        
        // Laser rovnou uděluje poškození všeho, čeho se dotýká
        if (!is_enemy) { check_powerup_collisions(p); }
        if (is_enemy) { check_player_collision(p); }

        // --- DYNAMICKÁ VÝŠKA LASERU ---
        int laser_height;
        
        if (!is_enemy) {
            // Hráčův laser vždy končí vizuálně u jeho lodě
            laser_height = SHIP_Y - 16;
        } else {
            // Nepřátelský laser
            // Zjistíme, jestli laser protíná naši loď v ose X 
            bool overlaps_ship = (p.x + 2 > x) && (p.x < x + SHIP_LENGTH);
            
            if (overlaps_ship) {
                // Laser nás trefuje! Zastavíme ho vizuálně nad lodí, aby ji nepřepsal.
                laser_height = SHIP_Y - 16; 
            } else {
                laser_height = BOT_SEP_LINE_Y - 16;
            }
        }

        // Paprsek se v ose Y neposouvá, jen trvá 0.5 sec
        if (micros() - p.spawn_time > 500000UL) {
            p.active = false;
            
            // Vypršel čas -> smažeme paprsek. 
            // Použijeme stejnou výšku, jakou jsme vypočítali!
            st7735_fill_rect(p.x, 16, 2, laser_height, COLOR_BG);
            update_inventory_ui(inventory_count, player_inventory);
            
            // Překreslíme power-upy ležící na ploše, protože je mohl laser graficky smazat
            for (int j = 0; j < MAX_POWERUPS; j++) {
                if (active_powerups[j].active) {
                    draw_powerup8x8(active_powerups[j].x, active_powerups[j].y, active_powerups[j].type, COLOR_BG);
                }
            }
        } else {
            // Hráčův paprsek bude modrozelený, nepřátelský třeba čistě červený
            uint16_t laser_color = is_enemy ? COLOR_RED : COLOR_CYAN;
            
            // Vykreslíme laser se správně vypočítanou výškou
            st7735_fill_rect(p.x, 16, 2, laser_height, laser_color);
        }
    }
}

/************************* spawn_enemy_projectile *************************/
void spawn_enemy_projectile(uint8_t received_x, WeaponType wep_type) {
	// 127 je šířka obrazovky. Tímto získáme přesně zrcadlovou pozici.
	uint8_t mirrored_x = (DISPLAY_WIDTH-1) - received_x;
	
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		// Hledáme volné místo ve vyhrazeném poli pro NEPŘÁTELE
		if (!enemy_lasers[i].active) {
			enemy_lasers[i].spawn_time = micros(); 
			enemy_lasers[i].active = true;
			enemy_lasers[i].x = mirrored_x;
			enemy_lasers[i].y = TOP_HUD_Y_END + 1; // Začíná nahoře
			enemy_lasers[i].type = wep_type; // Zbraň podle parametru
			
			// Pokud stál časovač střel (na obrazovce nic neletělo), spustíme ho
			if (projectileTimer->isDone()) {
				projectileTimer->startTimerUs(25000);
			}
			break; 
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
			player_lasers[i].type =  wep;
			player_lasers[i].spawn_time = micros(); // Pro laser

			// Zvuky přesunuty sem
			if (wep == WEP_RAILGUN) Soundboard::playSound(Soundboard::sfx_railgun);
			if (wep == WEP_ROCKET) Soundboard::playSound(Soundboard::sfx_rocket);
			if (wep == WEP_LASER) Soundboard::playSound(Soundboard::sfx_laser);
			if (wep == WEP_BURST) Soundboard::playSound(Soundboard::sfx_burst);

			// TADY ODESLAT DATA: Střela se právě narodila, řekneme to protihráči!
            // uart_send_projectile(player_lasers[i].x, wep);

			spawn_enemy_projectile(player_lasers[i].x, wep); //pro testovani, jestli se nepratelske strely vykresli na druhe strane - funguje

			break; // Vytvořeno, končíme hledání
		}
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
    
    x = DISPLAY_WIDTH / 2; // Startovní pozice lodě
	btn_state = BTN_IDLE;
	last_enc_btn = false;
	last_A = (PIND & (1 << PD2)) >> PD2;
	current_weapon = WEP_RAILGUN; // Aktuálně zvolená zbraň
	burst_shots_left = 0;
	inventory_count = 0;
	isShieldActive = false;
	isRapidFireActive = false;
	player_hp = 99;
	enemy_hp[1] = 99;
	enemy_hp[2] = 99;
    
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

	// Vyprázdníme HAL enkodér (zahodíme případné staré ticky)
	input_get_encoder_ticks();
    
    st7735_fill_screen(COLOR_BG);
    gameplay_draw_top_hud_static();
    gameplay_draw_top_hud_dynamic(enemy_hp[1], enemy_hp[2]); // P1_Hp, P2_Hp
    gameplay_draw_bottom_hud(99, WEP_RAILGUN, player_ammo); // your_hp, wep, ammo_array
	draw_dotted_rect(0, 78, 12, 33, COLOR_MAGENTA);
    draw_ship(x, SHIP_Y, COLOR_BLUE);
}

/************************* gameplay_tick *************************/
// Herní logika
GameState gameplay_tick(void) {

	// --- PŘEPÍNÁNÍ ZBRANÍ (enkodérové tlačítko) ---
	bool current_enc_btn = input_encoder_button_rising();
	if (current_enc_btn && !last_enc_btn) { // Detekce náběžné hrany
		WeaponType old_weapon = current_weapon;
		current_weapon = static_cast<WeaponType>((current_weapon + 1) % 4);
		draw_weapon_selection_box(old_weapon, COLOR_BG);
		draw_weapon_selection_box(current_weapon, COLOR_WHITE);
	}
	last_enc_btn = current_enc_btn;

	// --- ČTENÍ TLAČÍTKA STŘELBY (fire button přes HAL) ---
	bool fire_down = input_fire_button_pressed();

    switch (btn_state) {
	    case BTN_IDLE:
			if (fire_down && fireCooldownTimer->isDone()) {
                
                // --- START NÁHODY PŘI PRVNÍM VÝSTŘELU ---
                if (!is_rng_seeded) {
                    srand(micros());
                    is_rng_seeded = true;
                }

				btn_state = BTN_WAIT_PUSHED;
			}
			break;

	    case BTN_WAIT_PUSHED:
            if (fire_down) { 
                // Tlačítko je stále dole, začneme měřit dlouhý stisk
                holdTimer->startTimerUs(POWERUP_HOLD_TIME); // 600 ms pro aktivaci
                btn_state = BTN_WAIT_RELEASE;
            } else {
                btn_state = BTN_IDLE; // Falešný stisk
            }
            break;

        case BTN_WAIT_RELEASE:
            if (!fire_down) { 
                // 1. Tlačítko bylo PUŠTĚNO PŘED vypršením času -> KRÁTKÝ STISK (STŘELBA)
                
                // Zkontrolujeme cooldown zbraně TADY, těsně před výstřelem
                if (fireCooldownTimer->isDone()) {
                    try_shoot(current_weapon, x, SHIP_Y);
                    projectileTimer->startTimerUs(80000);
                    
                    uint32_t current_cd = isRapidFireActive ? COOLDOWNS_RAPID[current_weapon] : COOLDOWNS_NORMAL[current_weapon];
                    fireCooldownTimer->startTimerUs(current_cd);
                }
                
                btnTimer->startTimerUs(50000);
                btn_state = BTN_IDLE;
            }
            else if (holdTimer->isDone()) {
                // 2. Tlačítko je STÁLE DOLE a čas vypršel -> DLOUHÝ STISK (POWER-UP)
                use_powerup();
                
                // Přepneme do stavu, kde čekáme, až hráč tlačítko pustí (aby hned nestřílel)
                btn_state = BTN_WAIT_FULL_RELEASE;
            }
            break;

        case BTN_WAIT_FULL_RELEASE:
            if (!fire_down) {
                // Hráč konečně pustil tlačítko po aktivaci power-upu
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
		bool refresh_inventory = false;

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

		// --- ZÁCHRANA INVENTÁŘE ---
        if (refresh_inventory) {
            update_inventory_ui(inventory_count, player_inventory);
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
				// Smažeme věž z obrazovky
				st7735_fill_rect(active_sentries[i].x + 3, active_sentries[i].y, 8, 8, COLOR_BG);
				continue; // Přeskočíme zbytek smyčky pro tuto zničenou věž
			}
					
			// 2. Kontrola střelby
			if (current_time - active_sentries[i].last_shot_time > COOLDOWN_SENTRY) {
				// Věž střílí (jako typ pošleme Railgun z její pozice)
				spawn_projectile(WEP_RAILGUN, active_sentries[i].x, active_sentries[i].y);
						
				// Resetujeme časovač střelby
				active_sentries[i].last_shot_time = current_time;
			}
					
			// 3. Vykreslení věže
			draw_8bit_PROGMEM(active_sentries[i].x + 3, active_sentries[i].y, sentryGun_bitmap, COLOR_WHITE, COLOR_BG);
		}
	}
	
	// --- SPAWNOVÁNÍ NOVÝCH POWER-UPŮ ---
	if (powerupSpawnTimer->isDone()) {
		
		if (powerup_spawn_random()) {
			// ÚSPĚCH! Nastavíme další spawn za náhodný dlouhý čas (např. 8 až 24 sekund)
			uint32_t random_delay = 8000000UL + (rand() % 16) * 1000000UL; 
			powerupSpawnTimer->startTimerUs(random_delay);
		} else {
			// MAPA JE PLNÁ (funkce vrátila false). Zkusíme to znovu za 30 sec
			powerupSpawnTimer->startTimerUs(30000000UL);
		}
	}

	// --- UPDATE EXISTUJÍCÍCH POWER-UPŮ (VYPRŠENÍ ČASU) ---
	current_time = micros();
	for (int i = 0; i < MAX_POWERUPS; i++) {
		if (active_powerups[i].active) {
			
			// Čas života pevně na 10 sec
			if (current_time - active_powerups[i].spawn_time > 10000000UL) {
				active_powerups[i].active = false;
				// Smazání ikony z hrací plochy
				st7735_fill_rect(active_powerups[i].x, active_powerups[i].y, 8, 8, COLOR_BG);
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