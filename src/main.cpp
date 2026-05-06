#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "hal/spi.h"
#include "hal/st7735.h"
#include "Utilities/SoftwareTimer.hpp"

// Jednotliv� hern� moduly
#include "Graphics/Menu.hpp"
#include "Graphics/Graphics.hpp"
#include "Gameplay.hpp"
#include "Gamestate.hpp"
#include "Utilities/Soundboard.hpp"
#include <Communication/Datalink.hpp>
#include <hal/input.h>
#include <Communication/Messenger.hpp>

// Definice barev
#define COLOR_GREEN		0x07E0
#define COLOR_BLUE		0x001F
#define COLOR_RED		0xF800
#define COLOR_WHITE		0xFFFF
#define COLOR_CYAN		0x07FF
#define COLOR_ORANGE	0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_BG        0x0000 // Černé pozadí

void init_all(){
    spi_init();
	st7735_init();
    input_init();
	SoftwareTimerPool::initTimerPool();
    Soundboard::initSoundboard();
    Datalink::initDatalink();
}

void tick_all(){
    input_tick();
    SoftwareTimerPool::tick();
	Soundboard::play();
    Messenger::commLoop();
}

int main(void) {
	sei();
	
	// Všechny piny PD2 až PD5 nastavíme jako VSTUPY (0)
	DDRD &= ~((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5));
	// Zapneme Pull-up rezistory pro tyto piny
	PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5);

	
	
	// ��zení stavů
	GameState current_state = STATE_MENU;
	bool state_just_changed = true;
	
	// Soundboard
	

	while (1) {
		

		// HLAVN� STAVOV� AUTOMAT
		switch (current_state) {
			
			case STATE_MENU: { 
				if (state_just_changed) {
					menu_init();
					state_just_changed = false;
				}
				
				GameState next_state_menu = menu_tick();
				if (next_state_menu != current_state) {
					current_state = next_state_menu;
					state_just_changed = true;
				}
				break;
			} 

			case STATE_GAMEPLAY: { 
				if (state_just_changed) {
					gameplay_init();
					state_just_changed = false;
				}
				
				GameState next_state_game = gameplay_tick();
				if (next_state_game != current_state) {
					current_state = next_state_game;
					state_just_changed = true;
				}
				break;
			}

			case STATE_SCORE: { 
				if (state_just_changed) {
					st7735_fill_screen(COLOR_BG);
					state_just_changed = false;
				}
				break;
			}

			case STATE_ARSENAL: { // P�id�no, aby kompil�tor nenad�val
				break;
			}
		}
	}
}