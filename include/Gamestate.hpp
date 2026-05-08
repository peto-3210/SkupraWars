#pragma once
#include <hal/libraries.h>

typedef enum {
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_ARSENAL,  // unused
    STATE_SCORE,    // unused
    STATE_DEFEAT,
    STATE_VICTORY,
    STATE_DEAD_END,
    STATE_FATAL_ERROR
} GameState;