#pragma once
#include "Gamestate.hpp"

void gameplay_init(void);

//void process_projectile(Projectile& p, bool is_enemy);

GameState gameplay_tick(void);

//void add_ammo(WeaponType wep, uint8_t amount);