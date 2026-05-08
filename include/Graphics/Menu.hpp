#pragma once
#include "Gamestate.hpp"
#include "Gameplay.hpp"
#include "Graphics/Graphics.hpp"

// --- MENU CORE FUNCTIONS ---

/**
 * @brief Initializes the menu state and prepares the display.
 * * Sets up the environment for the main menu:
 * - **State Reset:** Resets the `selected_item` index to 0 (pointing to the first option).
 * - **Screen Clear:** Wipes the entire display using `COLOR_WHITE` to provide a 
 * high-contrast background for the menu UI.
 * - **First Render:** Calls `draw_menu()` to populate the screen with the initial 
 * list of options and the selector.
 * * @note This function should be called whenever the game returns to the 
 * main menu or upon initial power-up.
 */
void menu_init(void);


/**
 * @brief Advanced state machine handling menu navigation and setup.
 * * Manages the transition between different menu phases:
 * - **INIT_MENU:** Standard main menu selection (Start Game, etc.).
 * - **SELECT_PLAYER_NUM:** Allows user to pick the number of players using the encoder. 
 * Renders the current digit within a dotted decorative rectangle.
 * - **SELECT_NAME:** Cycles through a predefined list of names (strings) for the player profile.
 * * **Navigation Logic:** * - Uses `input_get_encoder_dir()` for scrolling through options/values.
 * - Uses `input_fire_button_rising()` to confirm selection and advance the state.
 * * @return GameState: Returns `STATE_MENU` while navigating, or `STATE_GAMEPLAY` 
 * when the setup is complete and the game should start.
 */
GameState menu_tick(void);


// --- GETTERS ---

/**
 * @brief Returns the index of the player name selected in the menu.
 * @return uint8_t Index into the 'names' array.
 */
uint8_t getSelectedName();


/**
 * @brief Returns the total number of players selected during setup.
 * @return uint8_t Number of players
 */
uint8_t getSelectedPlayerNum();


/**
 * @brief Retrieves a pointer to the string literal of a name by its index.
 * * This allows other modules to display the actual name string
 * instead of just its numeric ID.
 * * @param index The index of the name in the internal 'names' array.
 * @return const char* Pointer to the character string.
 */
const char* getName(uint8_t index);