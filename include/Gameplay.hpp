#pragma once
#include "Gamestate.hpp"
#include "Graphics/Graphics.hpp"

/************* ENUMY *************/
typedef enum { BTN_IDLE,
	BTN_WAIT_PUSHED,
	BTN_WAIT_RELEASE,
	BTN_WAIT_FULL_RELEASE
 } ButtonState;

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

void gameplay_init(void);

//void process_projectile(Projectile& p, bool is_enemy);

GameState gameplay_tick(void);

//void add_ammo(WeaponType wep, uint8_t amount);
void update_inventory_ui(uint8_t count, PowerUpType* inventory);
extern void draw_dotted_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
extern void draw_powerup8x8(uint8_t x, uint8_t y, PowerUpType type, uint16_t bg_color);
extern void gameplay_draw_bottom_hud(uint8_t your_health, WeaponType active_weapon, uint8_t ammo_counts[]);
extern void draw_weapon_selection_box(WeaponType wep, uint16_t color);
extern void gameplay_draw_top_hud_dynamic(uint8_t p1_health, uint8_t p2_health);
extern void gameplay_draw_top_hud_static(void);