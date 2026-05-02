#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "hal/spi.h"
#include "hal/st7735.h"
#include "SoftwareTimer.hpp"

#include "Gamestate.hpp"

// Jednotliv� hern� moduly
#include "Menu.hpp"
#include "Graphics.hpp"
#include "Gameplay.hpp"
#include "Soundboard.hpp"

int main(void) {
	sei();

	// Inicializace port�
	DDRB = 0b00100010;
	PORTB = 0b00000000;
	
	// OPRAVA: V�echny piny PD2 a� PD5 nastav�me jako VSTUPY (0)
	DDRD &= ~((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5));
	// Zapneme Pull-up rezistory pro tyto piny
	PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5);

	spi_init();
	st7735_init();
	SoftwareTimerPool::initTimerPool(10);
	
	// ��zen� stav�
	GameState current_state = STATE_MENU;
	bool state_just_changed = true;
	
	// Soundboard
	Soundboard::initSoundboard();

	while (1) {
		SoftwareTimerPool::tick();
		Soundboard::play();

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
					st7735_fill_screen(0x0000);
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
