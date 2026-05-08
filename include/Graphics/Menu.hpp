#pragma once
#include "Gamestate.hpp"
#include "Gameplay.hpp"
#include "Graphics/Graphics.hpp"

// --- MENU CORE FUNCTIONS ---

// Initializes the menu system and renders the initial UI
void menu_init(void);

// Handles menu navigation (scrolling) and item selection. 
// Called continuously in the main loop. Returns the next FSM state.
GameState menu_tick(void);


// --- GETTERS ---

// Returns the index of the selected player name
uint8_t getSelectedName();

// Returns the selected total number of players for the game
uint8_t getSelectedPlayerNum();

// Returns a pointer to the player name string at the given index
const char* getName(uint8_t index);