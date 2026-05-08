#include "Graphics/Menu.hpp"
#include "Gamestate.hpp"
#include "Gameplay.hpp"
#include "Graphics/Graphics.hpp"
#include "Utilities/SoftwareTimer.hpp"
#include "hal/st7735.h"
#include <avr/io.h>
#include <hal/input.h>


#define NUM_ITEMS 3
#define MAX_PLAYERS 8

const char* MENU_ITEMS[NUM_ITEMS] = {
    "PRIPOJIT",
    "ARZENAL", // unused
    "SKORE"    // unused
};

enum menuItems{
    START_GAME,
    SHOW_ARSENAL, // unused
    SHOW_SCORE    // unused
};

enum menuStates{
    INIT_MENU,
    SELECT_PLAYER_NUM,
    SELECT_NAME,
};

static uint8_t selected_item = 0;

const char* names[] = {
    "Abuk   ",
    "Sevak  ",
    "Ronda  ",
    "Amir   ",
    "Etera  ",
    "Lahemia",
    "Naftra ",
    "Nazked "
};
uint8_t player_name = 0;
uint8_t player_num = 0;
uint8_t getSelectedName(){return player_name;}
uint8_t getSelectedPlayerNum(){return player_num;}
const char* getName(uint8_t index){return names[index];}

menuStates menu_state = INIT_MENU;

/************************* draw_menu *************************/
// Function for a full (or partial) menu redraw
static void draw_menu() {
    // Draw the title
    draw_char_buffer(20, 10, "SKUPRA WARS", 0, COLOR_RED, COLOR_WHITE);
    
    // Draw the menu items
    for (int i = 0; i < NUM_ITEMS; i++) {
        uint8_t y_pos = 50 + (i * 20); 
        
        if (i == selected_item) {
            // Highlighted item
            draw_char_buffer(10, y_pos, ">", 0, COLOR_BG, COLOR_WHITE);
            draw_char_buffer(25, y_pos, MENU_ITEMS[i], 0, COLOR_BLUE, COLOR_WHITE); 
        } else {
            // Normal item
            draw_char_buffer(10, y_pos, " ", 0, COLOR_WHITE, COLOR_WHITE); // Clear the arrow
            draw_char_buffer(25, y_pos, MENU_ITEMS[i], 0, COLOR_BG, COLOR_WHITE); 
        }
    }
}

/************************* menu_init *************************/
// Init menu
void menu_init(void) {
    selected_item = 0;
    
    st7735_fill_screen(COLOR_WHITE);
    draw_menu();
}

/************************* menu_tick *************************/
// Menu state machine
GameState menu_tick(void) {
int8_t scroll_dir = input_get_encoder_dir();
    switch (menu_state){
        case INIT_MENU:
            if (scroll_dir < 0) {
                // Scrolling up
                if (selected_item > 0) {
                    selected_item--;
                    draw_menu(); // Redraw the change
                }
            } else if (scroll_dir > 0){
                // Scrolling down
                if (selected_item < NUM_ITEMS - 1) {
                    selected_item++;
                    draw_menu(); // Redraw the change
                }
            }
            
            // --- BUTTON HANDLING ---
            if (input_fire_button_rising() == true){
                switch (selected_item){
                    case START_GAME:
                        st7735_fill_screen(COLOR_WHITE);
                        draw_char_buffer(10, 10, "Vyber pocet hracu", 0, COLOR_RED, COLOR_WHITE); 
                        draw_dotted_rect(60, 50, 7, 9, COLOR_RED);
                        selected_item = 2;
                        char c = selected_item + 48;
                        draw_char_buffer(61, 51, &c, 1, COLOR_BG, COLOR_WHITE);
                        menu_state = SELECT_PLAYER_NUM;
                        break;
                }
            }
            break;

        case SELECT_PLAYER_NUM:
            if (scroll_dir < 0) {
                // Scrolling up
                if (selected_item > 2) {
                    selected_item--;
                    char c = selected_item + 48;
                    draw_char_buffer(61, 51, &c, 1, COLOR_BG, COLOR_WHITE);
                }
            } else if (scroll_dir > 0){
                // Scrolling down
                if (selected_item < MAX_PLAYERS - 1) {
                    selected_item++;
                    char c = selected_item + 48;
                    draw_char_buffer(61, 51, &c, 1, COLOR_BG, COLOR_WHITE);
                }
            }
            
            // --- BUTTON HANDLING ---
            if (input_fire_button_rising() == true){
                st7735_fill_screen(COLOR_WHITE);
                draw_char_buffer(20, 10, "Vyber si jmeno", 0, COLOR_RED, COLOR_WHITE); 
                player_num = selected_item;
                selected_item = 0;
                draw_char_buffer(50, 50, names[selected_item], 0, COLOR_RED, COLOR_WHITE);
                menu_state = SELECT_NAME;
            }
            break;


        case SELECT_NAME:
            if (scroll_dir < 0) {
                // Scrolling up
                if (selected_item > 0) {
                    selected_item--;
                    draw_char_buffer(50, 50, names[selected_item], 0, COLOR_RED, COLOR_WHITE);
                }
            } else if (scroll_dir > 0){
                // Scrolling down
                if (selected_item < MAX_PLAYERS - 1) {
                    selected_item++;
                    draw_char_buffer(50, 50, names[selected_item], 0, COLOR_RED, COLOR_WHITE);
                }
            }
            
            // --- BUTTON HANDLING ---
            if (input_fire_button_rising() == true){
                player_name = selected_item;
                st7735_fill_screen(COLOR_WHITE);
                return STATE_GAMEPLAY;          
            }
            break;
    }
        return STATE_MENU;
}