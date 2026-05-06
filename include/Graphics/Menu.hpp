#pragma once
#include "Gamestate.hpp"
#include "Gameplay.hpp"
#include "Graphics/Graphics.hpp"

// Inicializuje menu
void menu_init(void);

enum menuItems{
    START_GAME,
    SHOW_ARSENAL,
    SHOW_SCORE
};

// Stará se o scrollování a výběr
GameState menu_tick(void);