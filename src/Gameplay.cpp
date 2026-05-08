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
#include <Communication/Messenger.hpp>
#include <Graphics/Menu.hpp>

#define DISPLAY_LENGTH 160
#define DISPLAY_WIDTH 128

// Define HUD 
#define TOP_HUD_Y_START 0
#define TOP_HUD_Y_END   15
#define BOT_SEP_LINE_Y  140 
#define BOT_HUD_Y_START 141
#define SHIP_Y_TOP      124 

#define MAX_PROJECTILES 17 // can be modified up to user
#define RAILGUN_LENGTH 5
#define SHIP_LENGTH 16

// Power ups, can be modified up to user
#define MAX_INVENTORY 3
#define MAX_POWERUPS 4
#define POWERUP_HOLD_TIME  666000    // 666 ms
#define LIFETIME_RAPID    9000000    // 9.0 sec
#define LIFETIME_SHIELD   7000000    // 7.0 sec
#define LIFETIME_SENTRY   7500000    // 7.5 sec
#define COOLDOWN_SENTRY   1000000    // 1.0 sec

// Cooldowns of each weapon, can be modified up to user
const uint32_t COOLDOWNS_NORMAL[4] = {
	500000,  // WEP_RAILGUN  (0.5 sec)
	600000,  // WEP_BURST    (0.6 sec)
	800000,  // WEP_ROCKET   (0.8 sec)
	800000   // WEP_LASER    (0.8 sec)
};

// Cooldowns when rapid fire power-up is activated, can be modified up to user
const uint32_t COOLDOWNS_RAPID[4] = {
	400000,  // WEP_RAILGUN  (0.4 sec)
	450000,  // WEP_BURST    (0.45 sec)
	500000,  // WEP_ROCKET   (0.5 sec)
	600000   // WEP_LASER    (0.6 sec)
};

// Initial ammo of each weapon, can be modified up to user
uint8_t player_ammo[4] = {
	255,  // WEP_RAILGUN: does not matter, infinite ammo
	10,   // WEP_BURST
	10,   // WEP_ROCKET
	10    // WEP_LASER
};

// --- Local variables ---
WeaponType current_weapon;
static ButtonState btn_state;
PowerUpType player_inventory[MAX_INVENTORY];
SentryGun active_sentries[MAX_SENTRIES];
PowerUp active_powerups[MAX_POWERUPS];
static Projectile player_lasers[MAX_PROJECTILES];
static Projectile enemy_lasers[MAX_PROJECTILES];

bool isShieldActive;
bool isRapidFireActive; 
bool enemyHitOn = false;
bool fire_down = false;
bool endGame = false;
bool fatalError = false;
static bool last_enc_btn = false;
static bool is_rng_seeded = false;

uint8_t inventory_count;
uint16_t numberOfShots = 0;
uint16_t numberOfHits = 0;
uint16_t totalDamage = 0;
uint8_t burst_shots_left;
static uint8_t x; // Ship's position
static const uint8_t SHIP_Y = DISPLAY_LENGTH - 36;
static uint8_t last_A;
static uint8_t player_hp = 0;
static uint8_t enemy_hp[2] = {0, 0}; // [0] left, [1] right
//static char announcementBuffer[32] = {0};


// --- Timers ---
static SoftwareTimer* btnTimer = nullptr;
static SoftwareTimer* projectileTimer = nullptr;
static SoftwareTimer* fireCooldownTimer = nullptr;
static SoftwareTimer* holdTimer = nullptr;
static SoftwareTimer* rapidFirePUTimer = nullptr;
static SoftwareTimer* shieldPUTimer = nullptr;
static SoftwareTimer* powerupSpawnTimer = nullptr;
static SoftwareTimer* burstTimer = nullptr; 
static SoftwareTimer* hitEnemyTimer = nullptr;


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


/************************* update_hud_ammo *************************/
// Update ammo count after firing
void update_hud_ammo(WeaponType wep, uint8_t new_ammo) {
	// Railgun infty
	if (wep == WEP_RAILGUN) return;

	// Weapon HUD position
	uint8_t slot_x = 43 + static_cast<int>(wep) * 21;
	
	// Ammo count position
	uint8_t text_x = slot_x + 5;
	uint8_t text_y = BOT_HUD_Y_START + 11;

	// Clear old ammo
	st7735_fill_rect(text_x, text_y, 12, 8, COLOR_BG);

	char ammo_s[3];
	ammo_s[0] = (new_ammo / 10) + '0';
	ammo_s[1] = (new_ammo % 10) + '0';
	ammo_s[2] = '\0';
	
	// Renders new ammo
	draw_char_buffer(text_x, text_y, ammo_s, 0, COLOR_WHITE, COLOR_BG);
}

/************************* add_ammo *************************/
// Increase ammo after picking up power-up
void add_ammo(WeaponType wep, uint8_t amount) {
	player_ammo[wep] += amount;
	
	// Litim to 99
	if (player_ammo[wep] > 99) {
		player_ammo[wep] = 99;
	}

	// Visual update
    update_hud_ammo(wep, player_ammo[wep]);
}

/************************* add_powerup_to_inventory *************************/
// Add power-up into inventory (stack) and gives player random set amount of random ammo
void add_powerup_to_inventory(PowerUpType type) {
	if (inventory_count < MAX_INVENTORY) {
		// Add to stack
		player_inventory[inventory_count] = type;
		inventory_count++;
		
		// Renders power-up into player's inventory
		uint8_t draw_y = 100 - ((inventory_count - 1) * 10);
		draw_powerup8x8(2, draw_y, type, COLOR_BG);

		update_inventory_ui(inventory_count, player_inventory);
		
		// Not implemented
		// play sound power-up get
	} else {
		// Inventory is full - play sound
		// Not implemented
	}

    // --- Random ammo after each power-up ---
    
    // Random weapon (skip WEP_RAILGUN)
    WeaponType random_wep = static_cast<WeaponType>((rand() % 3) + 1);
    
    // Random amount of ammo (ei. 3 to 8), can be modified up to user
    uint8_t random_amount = 3 + (rand() % 5);
    add_ammo(random_wep, random_amount);
}

/************************* spawn_sentry_gun *************************/
// Place sentry gun onto battlefield
void spawn_sentry_gun(uint8_t ship_x) {
	for (int i = 0; i < MAX_SENTRIES; i++) {
		if (!active_sentries[i].active) {
			active_sentries[i].active = true;
			active_sentries[i].x = ship_x;
			
			// Place 10 pixels above ship
			active_sentries[i].y = SHIP_Y - 10;
			
			// Get current time for lifetime and cooldown
			uint32_t current_time = micros();
			active_sentries[i].spawn_time = current_time;
			active_sentries[i].last_shot_time = current_time;
			
			break; 
		}
	}
}

/************************* use_powerup *************************/
// Use and remove power-up from player's inventory
void use_powerup() {
	if (inventory_count > 0) {
		// Last picked up power-up
		inventory_count--;
		PowerUpType used_type = player_inventory[inventory_count];
		
		// Erase from inventory
		uint8_t erase_y = 100 - (inventory_count * 10);
		st7735_fill_rect(2, erase_y, 8, 8, COLOR_BG);
		
		// Activation and restart of specific timer
		switch(used_type) {
			case PU_RAPID_FIRE:
				isRapidFireActive = true;
				rapidFirePUTimer->startTimerUs(LIFETIME_RAPID);
				break;
			
			case PU_SHIELD:
				isShieldActive = true;
				shieldPUTimer->startTimerUs(LIFETIME_SHIELD);
				draw_ship(x, SHIP_Y, COLOR_WHITE);
				break;
			
			case PU_SENTRY:
				spawn_sentry_gun(x);
				break;
		}
		update_inventory_ui(inventory_count, player_inventory);
		Soundboard::playSound(Soundboard::sfx_powerup_use);
	}
}

/************************* powerup_spawn_random *************************/
// Spawns power-up on random set coordinates
bool powerup_spawn_random() {
	for (int i = 0; i < MAX_POWERUPS; i++) {
		if (!active_powerups[i].active) {
			active_powerups[i].active = true;
			
			// X position: from 14 to 114, can be modified up to user (not recommended)
			active_powerups[i].x = 14 + (rand() % 100);
			
			// Y position: from 20 to 100, can be modified up to user (not recommended)
			active_powerups[i].y = 20 + (rand() % 80);
			
			// Random power-up
			active_powerups[i].type = (PowerUpType)(rand() % 3);
			
			active_powerups[i].health = 11; // can be modified up to user (would also recommended change wep dmg with it)
			active_powerups[i].spawn_time = micros();
			
			draw_powerup8x8(active_powerups[i].x, active_powerups[i].y, active_powerups[i].type, COLOR_BG);
			
			return true;
		}
	}
	return false;
}

/************************* get_weapon_damage *************************/
// Get weapon DMG based on the type of a weapon, can be modified (recommended -> try different strategies :))
uint8_t get_weapon_damage(WeaponType wep) {
	switch(wep) {
		case WEP_RAILGUN: return 4;  // Power-up has 11 HP -> destroys in 3 shots
		case WEP_BURST:   return 3;  // Burst fires 3 projectiles, 9 DMG total
		case WEP_ROCKET:  return 11; // Set to 1-shot power-up
		case WEP_LASER:   return 1;  // This one is tricky, also uses i-frames in check_player_collision()
		default: return 1;
	}
}

/************************* check_collision *************************/
// AABB collisions, returns true if 2 rectangles are intercepting
bool check_collision(int x1, int y1, int w1, int h1,
					 int x2, int y2, int w2, int h2) {
	return (x1 < x2 + w2 && x1 + w1 > x2 &&
			y1 < y2 + h2 && y1 + h1 > y2);
}

/************************* check_powerup_collisions *************************/
// Processes collisions between a projectile and active power-ups
bool check_powerup_collisions(Projectile& p) {
	int px = p.x, py = p.y, pw = 1, ph = 1;
	
	// Define bounding box dimensions based on weapon type
	if (p.type == WEP_ROCKET) {
		pw = 3; ph = 8;
	} else if (p.type == WEP_LASER) {
		px = p.x - 1; pw = 2;
		py = 16; ph = SHIP_Y - 16;
	} else {
		// Railgun / Burst 
		pw = 1; ph = RAILGUN_LENGTH;
		py = p.y;
	}

	// Iterate through all power-up slots
	for (int i = 0; i < MAX_POWERUPS; i++) {
		if (active_powerups[i].active) {
			
			// Standard power-up collision area is 8x8 pixels
			if (check_collision(px, py, pw, ph, active_powerups[i].x, active_powerups[i].y, 8, 8)) {
				
				uint8_t dmg = get_weapon_damage(p.type);
				
				if (active_powerups[i].health <= dmg) {
					// Power-up destroyed -> Collect and add to player inventory
					active_powerups[i].active = false;
					st7735_fill_rect(active_powerups[i].x, active_powerups[i].y, 8, 8, COLOR_BG);
					add_powerup_to_inventory(active_powerups[i].type);

				} else {
					// Reduce health and trigger redraw to prevent visual "tearing" by the projectile
					active_powerups[i].health -= dmg;
					draw_powerup8x8(active_powerups[i].x, active_powerups[i].y, active_powerups[i].type, COLOR_BG);
				}

				// Handle projectile absorption (Laser is persistent and passes through)
                if (p.type != WEP_LASER) {
                    p.active = false;
                    
                    // Clear projectile sprite from display buffer to prevent "ghosting"
                    if (p.type == WEP_ROCKET) {
                        st7735_fill_rect(p.x, p.y, 3, 9, COLOR_BG);
                    } else {
                        st7735_fill_rect(p.x, p.y, 1, RAILGUN_LENGTH + 1, COLOR_BG);
                    }
                    return true; // Projectile consumed
                }
			}
		}
	}
	return false; // No collision detected, projectile continues to propagate
}

/************************* update_player_hp_ui *************************/
// Updates the player's health display on the HUD
void update_player_hp_ui(uint8_t hp) {
    char hp_str[4];
    // Format HP as a 2-digit string (e.g., "05", "99") to maintain UI alignment
    sprintf(hp_str, "%02d", hp); 

    // Renders the formatted HP string
    // String length is explicitly set to 2 for the fixed-width UI layout
    draw_char_buffer(18, BOT_HUD_Y_START + 6, hp_str, 2, COLOR_RED, COLOR_BG);
}

/************************* update_enemy_hp_ui *************************/
// Updates the health indicator for a specific enemy unit
void update_enemy_hp_ui(uint8_t enemy_index, uint8_t hp) {
    char hp_str[4];
    sprintf(hp_str, "%02d", hp);

    // Calculate horizontal positioning based on the enemy index:
    // Coordinates (18, 100) are calibrated for the current HUD layout
    int x_pos = (enemy_index == 0) ? 18 : 100;
	uint16_t color = (enemy_index == 0) ? COLOR_GREEN : COLOR_BLUE;

    draw_char_buffer(x_pos, TOP_HUD_Y_START + 4, hp_str, 2, color, COLOR_BG);
}

/************************* check_player_collision *************************/
// Evaluates collisions between enemy projectiles and the player's ship
bool check_player_collision(Projectile& p) {
	int px = p.x, py = p.y, pw = 1, ph = 1;
	
	// Align projectile hitbox with weapon-specific dimensions
	if (p.type == WEP_ROCKET) {
		pw = 3; ph = 8;
	} else if (p.type == WEP_LASER) {
		px = p.x - 1; pw = 2;
		py = 16; ph = SHIP_Y;
	} else {
		pw = 1; ph = RAILGUN_LENGTH;
		py = p.y;
	}

    // Check for collision with player ship (if shield is active, ship is invulnerable)
	if (check_collision(px, py, pw, ph, x, SHIP_Y, SHIP_LENGTH, SHIP_LENGTH)) {
		
		if (!isShieldActive) {
			
			// --- INVINCIBILITY FRAMES (I-FRAMES) LOGIC ---
            // Persistent timestamp to track the last registered hit
			static uint32_t last_damage_time = 0;
			bool can_take_damage = true;
			
			// Laser throttle: Implement damage interval (approx. 80ms) to prevent instant death
			if (p.type == WEP_LASER) {
				if (micros() - last_damage_time < 80000UL) {
					can_take_damage = false; 
				}
			}

			if (can_take_damage) {
				last_damage_time = micros();
				
				uint8_t dmg = get_weapon_damage(p.type); 

				// Prevent health underflow
				if (player_hp > dmg) {
					player_hp -= dmg;
				} else {
					player_hp = 0; 
				}

				// Update HUD and trigger audio feedback
				update_player_hp_ui(player_hp);			
				Soundboard::playSound(Soundboard::sfx_hit_enemy);
			}
		}

		// Synchronize player state over UART
		Messenger::sendHP(player_hp, x); 
		if (player_hp == 0){
			endGame = true;
		}
    
		// Projectile absorption (Persistent weapons like Laser bypass this)
		if (p.type != WEP_LASER) {
			p.active = false;

			// Clear projectile sprite
			if (p.type == WEP_ROCKET) {
				st7735_fill_rect(p.x, p.y, 3, 9, COLOR_BG);
			} else {
				// Note: Enemy railgun trails are rendered upwards (-j offset)
				for(uint8_t j = 0; j <= RAILGUN_LENGTH; j++) {
					st7735_draw_pixel(p.x, p.y - j, COLOR_BG);
				}
			}
			return true; // Collision handled, projectile consumed
		}
	}
	return false; // No impact detected
}

/************************* safe_draw_pixel *************************/
// Renders a single pixel with explicit bounds checking for the UI inventory area
void safe_draw_pixel(int x, int y, uint16_t color) {
    // Clipping logic: Discard pixels falling within the inventory bounds
    if (x <= 13 && y >= 77 && y <= 111) {
        return; 
    }
    st7735_draw_pixel(x, y, color);
}

/************************* safe_fill_rect *************************/
// Renders a filled rectangle with per-line clipping for the inventory zone
void safe_fill_rect(int x, int y, int w, int h, uint16_t color) {
    // Fast path: If the rectangle is outside the critical X-axis zone, render it entirely
    if (x > 13) {
        st7735_fill_rect(x, y, w, h, color);
        return;
    }
    
    // Slow path: Render line-by-line to handle partial clipping against the inventory HUD
    // This granular approach allows for smooth occlusion when objects enter the restricted zone
    for (int i = 0; i < h; i++) {
        int current_y = y + i;
        
        // Skip scanlines that overlap with the inventory vertical range
        if (current_y >= 77 && current_y <= 111) {
            continue; 
        }
        
        // Render a single validated scanline
        st7735_fill_rect(x, current_y, w, 1, color);
    }
}

/************************* redraw_powerups_under_tail *************************/
// Restores power-up sprites that were visually corrupted by a projectile's trail cleanup
void redraw_powerups_under_tail(int erase_x, int erase_y, int erase_w, int erase_h) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (active_powerups[i].active) {

            // Intersection check: Does the "dirty" erasure rectangle overlap the 8x8 power-up bounds?
            if (erase_x < active_powerups[i].x + 8 && erase_x + erase_w > active_powerups[i].x &&
                erase_y < active_powerups[i].y + 8 && erase_y + erase_h > active_powerups[i].y) {
                
                // Force an immediate redraw to restore visual integrity
                draw_powerup8x8(active_powerups[i].x, active_powerups[i].y, active_powerups[i].type, COLOR_BG);
            }
        }
    }
}

/************************* process_projectile *************************/
// Main update routine for all projectile types. Handles movement, collision detection, and optimized rendering
void process_projectile(Projectile& p, bool is_enemy) {
	
	// Determine movement vector and screen boundaries
	int direction = is_enemy ? 1 : -1;
	
	// --- 1/2. RAILGUN & BURST (Trail-based projectiles) ---
	if (p.type == WEP_RAILGUN || p.type == WEP_BURST) {
		
		// Calculate the projectile's tail position for efficient erasure
		int tail_offset = is_enemy ? -RAILGUN_LENGTH : RAILGUN_LENGTH;
		int tail_y = p.y + tail_offset;
		
		// Conditional cleanup of the trailing pixel to prevent "smearing"
		bool tail_on_screen = is_enemy ? (tail_y >= 16) : (tail_y < SHIP_Y);
		if (tail_on_screen) {
			safe_draw_pixel(p.x, tail_y, COLOR_BG);
			// Restore background assets (power-ups) if the enemy trail overlaps them
			if (is_enemy) redraw_powerups_under_tail(p.x, tail_y, 1, 1);
		}
		
		// Palette selection based on faction
		uint16_t trace_color = is_enemy ? COLOR_RED : COLOR_BLUE;
		uint16_t tip_color   = is_enemy ? COLOR_ORANGE : COLOR_CYAN;

		// Visual degradation: Fade the previous "tip" into a "trace" pixel
		safe_draw_pixel(p.x, p.y, trace_color);
		
		p.y += direction;

		// Collision logic branch
        if (!is_enemy && check_powerup_collisions(p)) { return; }
		if (is_enemy && check_player_collision(p)) { return; }
		
		// Render the new projectile head
		safe_draw_pixel(p.x, p.y, tip_color);

		// Boundary checks and final cleanup upon exit
        if (is_enemy) {
            if (p.y >= BOT_SEP_LINE_Y) {
                p.active = false;
                // Batch-erase the remaining projectile trace
                safe_fill_rect(p.x, p.y - RAILGUN_LENGTH, 1, RAILGUN_LENGTH + 1, COLOR_BG);
                redraw_powerups_under_tail(p.x, p.y - RAILGUN_LENGTH, 1, RAILGUN_LENGTH + 1);
                // Restore HUD separator integrity
                safe_draw_pixel(p.x, BOT_SEP_LINE_Y, COLOR_RED);
            }
        } else {
            if (p.y <= 16) {
                p.active = false;
                safe_fill_rect(p.x, p.y, 1, RAILGUN_LENGTH + 1, COLOR_BG);
            }
        }
    }
	
	// --- 3. ROCKET (Sprite-based with differential rendering) ---
    else if (p.type == WEP_ROCKET) {
        
        // Clean up the single line left behind by the moving sprite
        int erase_y = is_enemy ? (p.y) : (p.y + 7);
        if (erase_y >= 16 && (is_enemy || erase_y < SHIP_Y)) {
            safe_fill_rect(p.x, erase_y, 3, 1, COLOR_BG);
            if (is_enemy) redraw_powerups_under_tail(p.x, erase_y, 3, 1);
        }

        p.y += direction;

        if (!is_enemy && check_powerup_collisions(p)) { return; }
        if (is_enemy && check_player_collision(p)) { return; }

        uint16_t body_color = is_enemy ? COLOR_RED : COLOR_GREEN; 
        uint16_t wing_color = is_enemy ? COLOR_BLUE : COLOR_ORANGE; 
        int wing_y_offset   = is_enemy ? 0 : 4;          

        // Initial Full Render: Draw the entire sprite only during the first 50ms (spawn phase)
        if (micros() - p.spawn_time < 50000UL) {
            safe_fill_rect(p.x + 1, p.y, 1, 8, body_color);                    // Main fuselage
            safe_fill_rect(p.x, p.y + wing_y_offset, 1, 4, wing_color);        // Left stabilizer
            safe_fill_rect(p.x + 2, p.y + wing_y_offset, 1, 4, wing_color);    // Right stabilizer
        } else {
            // Incremental Update: Only draw the leading edge pixels
            if (is_enemy) {
                safe_draw_pixel(p.x + 1, p.y + 7, body_color);  // New fuselage tip
                safe_draw_pixel(p.x, p.y + 3, wing_color);      // New wing edge
                safe_draw_pixel(p.x + 2, p.y + 3, wing_color); 
            } else {
                safe_draw_pixel(p.x + 1, p.y, body_color);    
                safe_draw_pixel(p.x, p.y + 4, wing_color);    
                safe_draw_pixel(p.x + 2, p.y + 4, wing_color); 
            }
        }

        // Boundary exit logic
        if (is_enemy) {
            if ((p.y + 8) >= BOT_SEP_LINE_Y) {
                p.active = false;
                safe_fill_rect(p.x, p.y, 3, 8, COLOR_BG);
                redraw_powerups_under_tail(p.x, p.y, 3, 8);
                // safe_fill_rect(p.x, BOT_SEP_LINE_Y, 3, 1, COLOR_RED);
            }
        } else {
            if (p.y <= 16) {
                p.active = false;
                safe_fill_rect(p.x, p.y, 3, 8, COLOR_BG);
            }
        }
    }
	
	// --- 4. LASER (Persistent beam with dynamic height) ---
    else if (p.type == WEP_LASER) {
        
        // Continuous collision detection for persistent beams
        if (!is_enemy) { check_powerup_collisions(p); }
        if (is_enemy) { check_player_collision(p); }

        // --- Dynamic occlusion calculation ---
        int laser_height;
        
        if (!is_enemy) {
            laser_height = SHIP_Y - 16;
        } else {
            // Raycast check: Does the beam intersect the player's X-axis bounds?
            bool overlaps_ship = (p.x + 2 > x) && (p.x < x + SHIP_LENGTH);
            
            if (overlaps_ship) {
                // Adjust height to stop at ship hull (prevents overdrawing)
                laser_height = SHIP_Y - 16; 
            } else {
				// Extend to the bottom HUD separator
                laser_height = BOT_SEP_LINE_Y - 16;
            }
        }

        // Temporal logic: Handle beam expiration (500ms duration), can be modified up to user 
        if (micros() - p.spawn_time > 500000UL) {
            p.active = false;
            
            // Clear beam using the pre-calculated dynamic height
            st7735_fill_rect(p.x, 16, 2, laser_height, COLOR_BG);
            update_inventory_ui(inventory_count, player_inventory);
            
            // Re-draw all power-ups 
            for (int j = 0; j < MAX_POWERUPS; j++) {
                if (active_powerups[j].active) {
                    draw_powerup8x8(active_powerups[j].x, active_powerups[j].y, active_powerups[j].type, COLOR_BG);
                }
            }
        } else {
            uint16_t laser_color = is_enemy ? COLOR_RED : COLOR_CYAN;            
            // Draw active beam
            st7735_fill_rect(p.x, 16, 2, laser_height, laser_color);
        }
    }
}

/************************* spawn_enemy_projectile *************************/
// Spawns a projectile fired by the opponent
void spawn_enemy_projectile(uint8_t received_x, WeaponType wep_type) {
	// Invert X-axis to maintain spatial consistency across peers
	uint8_t mirrored_x = (DISPLAY_WIDTH-1) - received_x;
	
	// Locate an available slot in the enemy projectile buffer
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		if (!enemy_lasers[i].active) {
			enemy_lasers[i].spawn_time = micros(); 
			enemy_lasers[i].active = true;
			enemy_lasers[i].x = mirrored_x;
			enemy_lasers[i].y = TOP_HUD_Y_END + 1; // Spawn at the top HUD boundary
			enemy_lasers[i].type = wep_type; 
			
			// Wake up the global projectile update timer if idle (duty cycle 25ms)
			if (projectileTimer->isDone()) {
				projectileTimer->startTimerUs(25000);
			}
			break; 
		}
	}
}

/************************* spawn_projectile *************************/
// Spawns a local player projectile and synchronizes the event with the network peer
void spawn_projectile(WeaponType wep, uint8_t ship_x, uint8_t ship_y) {
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		if (!player_lasers[i].active) {
			player_lasers[i].active = true;

			// Center-align projectile relative to the ship sprite
			player_lasers[i].x = ship_x + 7; 

			// Adjust vertical origin based on projectile geometry
			player_lasers[i].y = (wep == WEP_ROCKET) ? (ship_y - 8) : (ship_y - 1);			
			player_lasers[i].type =  wep;
			player_lasers[i].spawn_time = micros(); // Mandatory for WEP_LASER

			// Execute weapon-specific audio feedback
			if (wep == WEP_RAILGUN) Soundboard::playSound(Soundboard::sfx_railgun);
			if (wep == WEP_ROCKET) Soundboard::playSound(Soundboard::sfx_rocket);
			if (wep == WEP_LASER) Soundboard::playSound(Soundboard::sfx_laser);
			if (wep == WEP_BURST) Soundboard::playSound(Soundboard::sfx_burst);

			// NETWORK SYNC: Notify the remote peer of the spawn event
            Messenger::sendProjectile(wep, ship_x + 7);
			numberOfShots++;

			break;
		}
	}
}

/************************* try_shoot *************************/
// Evaluates firing conditions and executes the shot if all requirements are met
void try_shoot(WeaponType wep, uint8_t ship_x, uint8_t ship_y) {
	// Determine the current projectile cap based on active power-ups
	uint8_t allowed_shots = isRapidFireActive ? 12 : 6;
	
	// Calculate current projectile load on screen
	uint8_t active_count = 0;
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		if (player_lasers[i].active) {
			active_count++;
		}
	}
	
	// Block firing if the maximum number of simultaneous projectiles is reached
	if (active_count >= allowed_shots) {
		Soundboard::playSound(Soundboard::sfx_noAmmo);
		return;
	}
	
	// Ammo availability check (Railgun is treated as a default weapon with infinite ammo)
	if (wep == WEP_RAILGUN || player_ammo[wep] > 0) {
		
		// Handle fire-mode logic
		if (wep == WEP_BURST) {
			// Instant trigger for the first projectile in a burst sequence
			spawn_projectile(WEP_BURST, ship_x, ship_y);
			burst_shots_left = 2;
			burstTimer->startTimerUs(300000); 
		} else {
			spawn_projectile(wep, ship_x, ship_y);
		}

		// Decrement ammo count and synchronize HUD
		if (wep != WEP_RAILGUN) {
			player_ammo[wep]--;
			update_hud_ammo(wep, player_ammo[wep]);
		}
	} else {
		// Out-of-ammo state
		Soundboard::playSound(Soundboard::sfx_noAmmo);
	}
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


/************************* gameplay_init *************************/
// Initializes the gameplay scene, resets state variables, and handles resource allocation
void gameplay_init(void) {
	
	// --- RESOURCE ALLOCATION ---
    // Allocate hardware-abstracted software timers only if they don't exist
	if (btnTimer == nullptr) {
		btnTimer = SoftwareTimerPool::acquireTimer();
		projectileTimer = SoftwareTimerPool::acquireTimer();
		burstTimer = SoftwareTimerPool::acquireTimer();
		fireCooldownTimer = SoftwareTimerPool::acquireTimer();
		holdTimer = SoftwareTimerPool::acquireTimer();
		rapidFirePUTimer = SoftwareTimerPool::acquireTimer();
		shieldPUTimer = SoftwareTimerPool::acquireTimer();
		powerupSpawnTimer = SoftwareTimerPool::acquireTimer();
		hitEnemyTimer = SoftwareTimerPool::acquireTimer();
	}
	
	// Initialize primary game loop and spawn timers
	burstTimer->startTimerUs(100);
	fireCooldownTimer->startTimerUs(0);
	powerupSpawnTimer->startTimerUs(15000000);
    
	// --- PLAYER STATE RESET ---
    x = DISPLAY_WIDTH / 2; // Default horizontal ship alignment
	btn_state = BTN_IDLE;
	last_enc_btn = false;

	// Initial hardware register read for the rotary encoder A-channel
	last_A = (PIND & (1 << PD2)) >> PD2;

	current_weapon = WEP_RAILGUN;
	burst_shots_left = 0;
	inventory_count = 0;
	isShieldActive = false;
	isRapidFireActive = false;
	player_hp = 99;   // can be modified up to user
	enemy_hp[1] = 99; // can be modified up to user
	enemy_hp[2] = 99; // can be modified up to user
    
	// --- MEMORY CLEANUP & OBJECT POOL RESET ---
    // Zero out active flags for all entity buffers to prevent stale data
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

	// --- RENDER BOOTSTRAP ---
	st7735_fill_screen(COLOR_BG);
	draw_ship(50, 50, COLOR_ORANGE);
	draw_char_buffer(10, 10, "Cekani na hrace", 0, COLOR_GREEN, COLOR_BG);

	// Network Topology Handshake
	if (Messenger::initTopology(2, getSelectedName()) == false){
		fatalError = true;
		st7735_fill_screen(COLOR_BG);
		return;
		Messenger::initializedTopology = true;
	} 
	else {
		draw_char_buffer(100, SHIP_Y, "INIT", 0, COLOR_GREEN, COLOR_BG);
	}

	// Play welcome music
	Soundboard::playMelody(Soundboard::imperialMarch);
    
	// UI Assembly: Render static and dynamic HUD elements
    st7735_fill_screen(COLOR_BG);
    gameplay_draw_top_hud_static();
    gameplay_draw_top_hud_dynamic(enemy_hp[1], enemy_hp[2]); 
    gameplay_draw_bottom_hud(player_hp, WEP_RAILGUN, player_ammo); 

	draw_dotted_rect(0, 78, 12, 33, COLOR_MAGENTA); // Inventory slot outline
    draw_ship(x, SHIP_Y, COLOR_BLUE);

	// Stability delay after resource-heavy initialization
	SoftwareTimerPool::busyWaitUs(5000);
}


/************************* gameplay_tick *************************/
// Main gameplay update loop. Executed once per frame. Handles network synchronization, input processing, and player actions.
GameState gameplay_tick(void) {

	// --- TERMINAL STATE CHECKS ---
	if (fatalError == true){
		return STATE_FATAL_ERROR;
	}

	if (endGame == true){
		return STATE_DEFEAT;
	}

	// --- NETWORK PACKET DISPATCHING ---
	uint8_t annP;
	
	// Process game announcements (e.g., player deaths)
	if (Messenger::getAnnouncement(annP) == Messenger::deathAnn){
		//uint8_t killer; // not implemented
		uint8_t victim;
		//killer = annP >> 3; // not implemented
		victim = annP & 0b111;
		// If no more neighbors are active, transition to victory state
		if (Messenger::disableNeighbour(victim) == false){
			return STATE_VICTORY;
		}
	}

	Datalink::packetPayload p;
	bool direction;
	uint8_t position = 0;

	// Process incoming functional packets from the network
	switch (Messenger::getPacket(p, direction)){
		case Messenger::shootProjectileFun:
			position = p.projectile.position;
			// Handle coordinate mapping between peers
			if (direction == false){
				position += 64;
			}
			spawn_enemy_projectile(position, (WeaponType)p.projectile.type);
			break;

		case Messenger::tellHPFun:
			update_enemy_hp_ui(!direction, p.hp.value);
			// Visual feedback for successful hits on remote targets
			if (p.hp.youHitMe == true){
				draw_enemy_hit(direction);
				hitEnemyTimer->startTimerUs(250000);
				enemyHitOn = true;
				numberOfHits++;
				totalDamage += enemy_hp[direction] - p.hp.value;
			}
			break;
	}

	// Clear enemy hit visualization after timer expiration
	if (enemyHitOn == true && hitEnemyTimer->isDone()){
		enemyHitOn = false;
		clear_enemy_hit();
	}

	// --- INPUT: WEAPON SELECTION (Encoder Button) ---
	bool current_enc_btn = input_encoder_button_rising();
	if (current_enc_btn && !last_enc_btn) { // Rising edge detection
		WeaponType old_weapon = current_weapon;
		// Cycle through available weapon types
		current_weapon = static_cast<WeaponType>((current_weapon + 1) % 4);

		// Update UI selection highlights
		draw_weapon_selection_box(old_weapon, COLOR_BG);
		draw_weapon_selection_box(current_weapon, COLOR_WHITE);
	}
	last_enc_btn = current_enc_btn;

	// --- INPUT: FIRE BUTTON (HAL Abstraction) ---
	if (fire_down == false){
		fire_down = input_fire_button_rising();
	} else {
		fire_down = input_fire_button_pressed();
	}

	// --- FIRE BUTTON FINITE STATE MACHINE (FSM) ---
    switch (btn_state) {
	    case BTN_IDLE:
			if (fire_down && fireCooldownTimer->isDone()) {
				// Initialize RNG on first user interaction for better entropy
                if (!is_rng_seeded) {
                    srand(micros());
                    is_rng_seeded = true;
                }
				btn_state = BTN_WAIT_PUSHED;
			}
			break;

	    case BTN_WAIT_PUSHED:
            if (fire_down) { 
                // Button held: initiate long-press timer for power-up activation
                holdTimer->startTimerUs(POWERUP_HOLD_TIME); 
                btn_state = BTN_WAIT_RELEASE;
            } else {
                btn_state = BTN_IDLE; // Debounce or false trigger
            }
            break;

        case BTN_WAIT_RELEASE:
            if (!fire_down) { 
                // Scenario 1: Short press detected (standard fire action)
                if (fireCooldownTimer->isDone()) {
                    try_shoot(current_weapon, x, SHIP_Y);
                    projectileTimer->startTimerUs(80000);
                    
					// Apply dynamic cooldown based on active buffs
                    uint32_t current_cd = isRapidFireActive ? COOLDOWNS_RAPID[current_weapon] : COOLDOWNS_NORMAL[current_weapon];
                    fireCooldownTimer->startTimerUs(current_cd);
                }
                
                btnTimer->startTimerUs(50000);
                btn_state = BTN_IDLE;
            }
            else if (holdTimer->isDone()) {
                // Scenario 2: Long press duration reached (activate power-up)
                use_powerup();
                
                // Prevent accidental firing: wait for complete button release
                btn_state = BTN_WAIT_FULL_RELEASE;
            }
            break;

        case BTN_WAIT_FULL_RELEASE:
            if (!fire_down) {
                btn_state = BTN_IDLE;
            }
            break;
    } 


    // --- BURST SEQUENCE LOGIC ---
	// Handles subsequent projectiles in a burst-fire sequence based on a timed interval
    if (burst_shots_left > 0 && burstTimer->isDone()) {
		spawn_projectile(WEP_BURST, x, SHIP_Y);
		burst_shots_left--;

		if (burst_shots_left > 0) {
			burstTimer->startTimerUs(250000); // 250ms interval between burst rounds, can be modified (not recommended)
		}
	}

    // --- PROJECTILE PHYSICS & RENDERING PIPELINE ---
    if (projectileTimer->isDone()) {
	    bool any_laser_active = false;
		bool refresh_inventory = false;

		// A) PLAYER PROJECTILE UPDATE
		for (int i = 0; i < MAX_PROJECTILES; i++) {
			if (player_lasers[i].active) {
				any_laser_active = true;
				process_projectile(player_lasers[i], false); // false = source is player
			}
		}

		// B) ENEMY PROJECTILE UPDATE
		for (int i = 0; i < MAX_PROJECTILES; i++) {
			if (enemy_lasers[i].active) {
				any_laser_active = true;
				process_projectile(enemy_lasers[i], true); // true = source is enemy
			}
		}

        // Restore inventory UI if graphical corruption was detected
        if (refresh_inventory) {
            update_inventory_ui(inventory_count, player_inventory);
        }

	    // Maintain the update cycle if at least one projectile is traversing the playfield
	    if (any_laser_active) {
		    projectileTimer->startTimerUs(25000);
	    }
    }
	
	uint32_t current_time = micros();

	// --- SENTRY GUN AUTONOMOUS LOGIC ---
	for (int i = 0; i < MAX_SENTRIES; i++) {
		if (active_sentries[i].active) {
					
			// Lifetime expiration check
			if (current_time - active_sentries[i].spawn_time > LIFETIME_SENTRY) {
				active_sentries[i].active = false;
				// De-render sentry sprite
				st7735_fill_rect(active_sentries[i].x + 3, active_sentries[i].y, 8, 8, COLOR_BG);
				continue; 
			}
					
			// Fire-rate control
			if (current_time - active_sentries[i].last_shot_time > COOLDOWN_SENTRY) {
				// Sentry triggers a standard Railgun projectile from its current position
				spawn_projectile(WEP_RAILGUN, active_sentries[i].x, active_sentries[i].y);
				active_sentries[i].last_shot_time = current_time;
			}
					
			// Sprite rendering
			draw_8bit_PROGMEM(active_sentries[i].x + 3, active_sentries[i].y, sentryGun_bitmap, COLOR_WHITE, COLOR_BG);
		}
	}
	
	// --- POWER-UP SPAWN SCHEDULER ---
	if (powerupSpawnTimer->isDone()) {
		if (powerup_spawn_random()) {
			// SUCCESS: Schedule next spawn with a random interval (8s - 24s) for gameplay variance, can be modified up to user
			uint32_t random_delay = 8000000UL + (rand() % 16) * 1000000UL; 
			powerupSpawnTimer->startTimerUs(random_delay);
		} else {
			// POOL EXHAUSTED: Field is full, retry in 30s
			powerupSpawnTimer->startTimerUs(30000000UL);
		}
	}

	// --- POWER-UP DESPAWN LOGIC ---
	current_time = micros();
	for (int i = 0; i < MAX_POWERUPS; i++) {
		if (active_powerups[i].active) {
			// Fixed 10s expiration to maintain field cleanliness, can be modified up to user
			if (current_time - active_powerups[i].spawn_time > 10000000UL) {
				active_powerups[i].active = false;
				// Remove sprite from playfield
				st7735_fill_rect(active_powerups[i].x, active_powerups[i].y, 8, 8, COLOR_BG);
			}
		}
	}


    // --- INPUT: ROTARY ENCODER (Ship Movement) ---
    // Read Phase A from Port D registers
    uint8_t current_A = (PIND & (1 << PD3)) >> PD3;

    if (current_A != last_A) {
	    if (current_A == 0) { // Trigger movement on falling edge of Phase A
		    uint8_t current_B = (PIND & (1 << PD2)) >> PD2;
		    uint8_t old_x = x; 

		    // --- SPATIAL NAVIGATION LOGIC (With Screen Wrapping) ---
		    if (current_B != current_A) {
				// Direction: Left
			    if (x >= 3) {
				    x -= 3; 
				} else {
				    x = DISPLAY_WIDTH - SHIP_LENGTH; // Wrap to right boundary
			    }
			} else {
				// Direction: Right
				if (x <= (DISPLAY_WIDTH - SHIP_LENGTH - 3)) {
					x += 3; 
				} else {
					x = 0; // Wrap to left boundary
				}
		    }

		    // --- RENDERING (Dirty Region Clearing) ---
            // Only update the display if movement occurred
		    if (x != old_x) {
			    
			    // Case A: Teleport / Wrap-around (Full sprite clearance required)
			    if ((old_x < 3 && x > 100) || (old_x > 100 && x < 3)) {
				    // Smažeme celý starý prostor 16x16
				    st7735_fill_rect(old_x, SHIP_Y, SHIP_LENGTH, SHIP_LENGTH, COLOR_BG);
			    }
			    // Case B: Progressive Move Left (Clear 3px trail on the trailing edge)
			    else if (x < old_x) {
				    st7735_fill_rect(old_x + 13, SHIP_Y, 3, SHIP_LENGTH, COLOR_BG);
			    }
			    // Case C: Progressive Move Right (Clear 3px trail on the leading edge)
			    else {
				    st7735_fill_rect(old_x, SHIP_Y, 3, SHIP_LENGTH, COLOR_BG);
			    }
			    
			    // Commit ship sprite with context-aware coloring
				uint16_t ship_color = isShieldActive ? COLOR_WHITE : COLOR_BLUE; 
			    draw_ship(x, SHIP_Y, ship_color);
		    }
	    }
	    last_A = current_A;
    }	
	
	// --- POWER-UP LIFECYCLE MANAGEMENT ---
	// Rapid Fire status check
	if (isRapidFireActive && rapidFirePUTimer->isDone()) {
		isRapidFireActive = false;
	}
	
	// Shield status check and visual revert
	if (isShieldActive && shieldPUTimer->isDone()) {
		isShieldActive = false;
		draw_ship(x, SHIP_Y, COLOR_BLUE);
	}
	
	return STATE_GAMEPLAY; // Cycle complete, maintain current state
}