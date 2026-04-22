#include "Menu.hpp"
#include "Gamestate.hpp"
#include "Gameplay.hpp"
#include "Graphics.hpp"
#include "SoftwareTimer.hpp"
#include "hal/st7735.h"
#include <avr/io.h>

// --- NASTAVENÍ POLOŽEK MENU ---
#define NUM_ITEMS 3
const char* MENU_ITEMS[NUM_ITEMS] = {
    "PRIPOJIT",
    "ARZENAL",
    "SKORE"
};

// --- LOKÁLNÍ PROMĚNNÉ ---
static uint8_t selected_item = 0; // Která položka je právě vybraná
static uint8_t last_enc_A;        // Pro čtení enkodéru

// Můžeme recyklovat tvůj debounce z gameplaye
typedef enum { MENU_BTN_IDLE, MENU_BTN_WAIT_DEBOUNCE, MENU_BTN_WAIT_RELEASE } MenuButtonState;
static MenuButtonState btn_state = MENU_BTN_IDLE;
static SoftwareTimer* menuBtnTimer;

// Funkce pro kompletní (nebo částečné) překreslení menu
static void draw_menu() {
    // Vykreslíme nadpis
    draw_string(20, 10, "SKUPRA WARS", 0xF800, 0xFFFF); // Příklad: červený text na bílém
    
    // Vykreslíme položky
    for (int i = 0; i < NUM_ITEMS; i++) {
        uint8_t y_pos = 50 + (i * 20); // Každá položka je o 20 pixelů níž
        
        if (i == selected_item) {
            // Zvýrazněná položka (např. inverzní barvy nebo přidáme šipku ">")
            draw_string(10, y_pos, ">", 0x0000, 0xFFFF);
            draw_string(25, y_pos, MENU_ITEMS[i], 0x001F, 0xFFFF); // Modrá barva
        } else {
            // Normální položka
            draw_string(10, y_pos, " ", 0xFFFF, 0xFFFF); // Smazání šipky
            draw_string(25, y_pos, MENU_ITEMS[i], 0x0000, 0xFFFF); // Černá barva
        }
    }
}

void menu_init(void) {
    menuBtnTimer = SoftwareTimerPool::acquireTimer();
    selected_item = 0;
    last_enc_A = (PIND & (1 << PD2)) >> PD2;
    
    st7735_fill_screen(0xFFFF); // Bílé pozadí
    draw_menu();
}

GameState menu_tick(void) {
    // --- 1. ČTENÍ ENKODÉRU (SCROLLOVÁNÍ) ---
    uint8_t current_enc_A = (PIND & (1 << PD2)) >> PD2;

    if (current_enc_A != last_enc_A) {
        if (current_enc_A == 0) {
            uint8_t current_enc_B = (PIND & (1 << PD3)) >> PD3;
            
            if (current_enc_B != current_enc_A) {
                // Scrollování nahoru
                if (selected_item > 0) {
                    selected_item--;
                    draw_menu(); // Překreslíme změnu
                }
            } else {
                // Scrollování dolů
                if (selected_item < NUM_ITEMS - 1) {
                    selected_item++;
                    draw_menu(); // Překreslíme změnu
                }
            }
        }
        last_enc_A = current_enc_A;
    }
    
    // --- 2. ČTENÍ TLAČÍTKA (POTVRZENÍ) ---
    switch (btn_state) {
        case MENU_BTN_IDLE:
            if (!(PIND & (1 << PD5))) {
                menuBtnTimer->startTimerUs(50000); // 50ms debounce
                btn_state = MENU_BTN_WAIT_DEBOUNCE;
            }
            break;

        case MENU_BTN_WAIT_DEBOUNCE:
            if (menuBtnTimer->isDone()) {
                if (!(PIND & (1 << PD5))) {
                    
                    if (selected_item == 0) { // Kliknuto na "PRIPOJIT"
	                    gameplay_init();      // Než přepneme, nastavíme hru do výchozího stavu
	                    btn_state = MENU_BTN_WAIT_RELEASE;
	                    return STATE_GAMEPLAY; 
                    }
                    
                    btn_state = MENU_BTN_WAIT_RELEASE;
                } else {
                    btn_state = MENU_BTN_IDLE;
                }
            }
            break;

        case MENU_BTN_WAIT_RELEASE:
            if (PIND & (1 << PD5)) {
                btn_state = MENU_BTN_IDLE;
            }
            break;
    }
	
	return STATE_MENU; // Pokud se nic nezměnilo, zůstáváme v menu
}