#pragma once
#include "Gamestate.hpp"
#include "Gameplay.hpp"
#include "Graphics/Graphics.hpp"

// Inicializuje menu
void menu_init(void);

// Stará se o scrollování a výběr
GameState menu_tick(void);

uint8_t getSelectedName();
uint8_t getSelectedPlayerNum();