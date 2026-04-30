#pragma once
#include "Gamestate.hpp"

// Inicializuje menu (vykreslí ho poprvé)
void menu_init(void);

// Stará se o scrollování a výběr
GameState menu_tick(void);