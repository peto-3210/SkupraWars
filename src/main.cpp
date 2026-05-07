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
#include <stdio.h>

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
	init_all();
	// Všechny piny PD2 až PD5 nastavíme jako VSTUPY (0)
	//DDRD &= ~((1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5));
	// Zapneme Pull-up rezistory pro tyto piny
	//PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD4) | (1 << PD5);

	
	
	// ��zení stavů
	GameState current_state = STATE_MENU;
	bool state_just_changed = true;
	extern	uint16_t numberOfShots;
	extern	uint16_t numberOfHits;
	extern	uint16_t totalDamage;
	
	/*if (Messenger::initTopology(3, 0x0004) == false){
		Soundboard::playSound(Soundboard::sfx_laser);
	}
	else {
		Soundboard::playSound(Soundboard::sfx_rocket);
	}*/
	

	while (1) {
		tick_all();

		if (input_reset_button_rising() == true){
			current_state = STATE_MENU;
			state_just_changed = true;
			numberOfShots = 0;
			numberOfHits = 0;
			totalDamage = 0;
		}

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
					st7735_fill_screen(COLOR_MAGENTA);
					state_just_changed = false;
				}
				break;
			}

			case STATE_ARSENAL: { // P�id�no, aby kompil�tor nenad�val
				break;
			}

			case STATE_DEFEAT: {
				st7735_fill_screen(COLOR_RED);
				draw_char_buffer(10, 20, getName(getSelectedName()), 0, COLOR_WHITE, COLOR_BG);
				draw_char_buffer(10, 10, "Umrel(a)s!", 0, COLOR_WHITE, COLOR_BG);
				draw_ship(56, 50, COLOR_MAGENTA);

				char buf[16];
				snprintf_P(buf, sizeof(buf), PSTR("Zasahy: %u"), numberOfHits);
				draw_char_buffer(10, 90, buf, 0, COLOR_WHITE, COLOR_BG);
				snprintf_P(buf, sizeof(buf), PSTR("Vystrely: %u"), numberOfShots);
				draw_char_buffer(10, 80, buf, 0, COLOR_WHITE, COLOR_BG);
				Soundboard::playMelody(Soundboard::gameOver);
				current_state = STATE_DEAD_END;
				break;
			}

			case STATE_VICTORY: {
				st7735_fill_screen(COLOR_GREEN);
				draw_char_buffer(10, 20, getName(getSelectedName()), 0, COLOR_WHITE, COLOR_BG);
				draw_char_buffer(10, 10, "Vyhral(a)s!", 0, COLOR_WHITE, COLOR_BG);
				draw_ship(56, 50, COLOR_CYAN);

				char buf[16];
				snprintf_P(buf, sizeof(buf), PSTR("Zasahy: %u"), numberOfHits);
				draw_char_buffer(10, 90, buf, 0, COLOR_WHITE, COLOR_BG);
				snprintf_P(buf, sizeof(buf), PSTR("Vystrely: %u"), numberOfShots);
				draw_char_buffer(10, 80, buf, 0, COLOR_WHITE, COLOR_BG);
				//snprintf(buf, sizeof(buf), "Poskozeni: %d", totalDamage);
				//draw_char_buffer(10, 100, buf, 15, COLOR_WHITE, COLOR_BG);
				Soundboard::playMelody(Soundboard::grandVictory);
				current_state = STATE_DEAD_END;
			}break;

			case STATE_FATAL_ERROR:
				draw_char_buffer(20, 50, "FATAL ERROR", 0, COLOR_WHITE, COLOR_BG);
				break;

			case STATE_DEAD_END:
			break;


		}
	}
}