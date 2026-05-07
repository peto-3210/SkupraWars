#pragma once
#include <hal/libraries.h>
typedef enum {
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_ARSENAL, // Zatím nevyužito
    STATE_SCORE,    // Zatím nevyužito
    STATE_DEFEAT,
    STATE_VICTORY
} GameState;