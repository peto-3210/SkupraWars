#include "Graphics/Menu.hpp"
#include "Gamestate.hpp"
#include "Gameplay.hpp"
#include "Graphics/Graphics.hpp"
#include "Utilities/SoftwareTimer.hpp"
#include "hal/st7735.h"
#include <avr/io.h>
#include <hal/input.h>

// --- NASTAVENÍ POLOŽEK MENU ---
#define NUM_ITEMS 3

const char* MENU_ITEMS[NUM_ITEMS] = {
    "PRIPOJIT",
    "ARZENAL",
    "SKORE"
};

// --- LOKÁLNÍ PROMĚNNÉ ---
static uint8_t selected_item = 0; // Která položka je právě vybraná

// Funkce pro kompletní (nebo částečné) překreslení menu
static void draw_menu() {
    // Vykreslíme nadpis
    draw_char_buffer(20, 10, "SKUPRA WARS", 0, 0xF800, 0xFFFF); // Příklad: červený text na bílém
    
    // Vykreslíme položky
    for (int i = 0; i < NUM_ITEMS; i++) {
        uint8_t y_pos = 50 + (i * 20); // Každá položka je o 20 pixelů níž
        
        if (i == selected_item) {
            // Zvýrazněná položka (např. inverzní barvy nebo přidáme šipku ">")
            draw_char_buffer(10, y_pos, ">", 0, 0x0000, 0xFFFF);
            draw_char_buffer(25, y_pos, MENU_ITEMS[i], 0, 0x001F, 0xFFFF); // Modrá barva
        } else {
            // Normální položka
            draw_char_buffer(10, y_pos, " ", 0, 0xFFFF, 0xFFFF); // Smazání šipky
            draw_char_buffer(25, y_pos, MENU_ITEMS[i], 0, 0x0000, 0xFFFF); // Černá barva
        }
    }
}

void menu_init(void) {
    selected_item = 0;
    
    st7735_fill_screen(0xFFFF); // Bílé pozadí
    draw_menu();
}


GameState menu_tick(void) {
    // --- 1. ČTENÍ ENKODÉRU (SCROLLOVÁNÍ) ---
    int8_t ticks = input_get_encoder_ticks();
    if (ticks < 0) {
        // Scrollování dolů
        if (selected_item < NUM_ITEMS - 1) {
            selected_item++;
            draw_menu();
        }
    } else if (ticks > 0) {
        // Scrollování nahoru
        if (selected_item > 0) {
            selected_item--;
            draw_menu();
        }
    }
    
    // --- 2. ČTENÍ TLAČÍTKA (POTVRZENÍ) ---
    if (input_encoder_button_rising() == true){
        switch (selected_item){
            case START_GAME:
                return STATE_GAMEPLAY;
                break;
        }
    }
    
    return STATE_MENU;
}