#pragma once
#include "Gamestate.hpp"
#include "Gameplay.hpp"
#include "Graphics/Graphics.hpp"

// Recyklujeme debounce z gameplaye
typedef enum {
    MENU_BTN_IDLE,
    MENU_BTN_WAIT_DEBOUNCE,
    MENU_BTN_WAIT_RELEASE
} MenuButtonState;

// Inicializuje menu
void menu_init(void);

// Stará se o scrollování a výběr
GameState menu_tick(void);