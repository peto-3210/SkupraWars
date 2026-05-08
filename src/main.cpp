#define F_CPU 16000000UL // Defines the CPU clock frequency
#include <avr/io.h>
#include <avr/interrupt.h>
#include "hal/spi.h"
#include "hal/st7735.h"
#include "Utilities/SoftwareTimer.hpp"
#include "Graphics/Menu.hpp"
#include "Graphics/Graphics.hpp"
#include "Gameplay.hpp"
#include "Gamestate.hpp"
#include "Utilities/Soundboard.hpp"
#include <Communication/Datalink.hpp>
#include <hal/input.h>
#include <Communication/Messenger.hpp>
#include <stdio.h>

// --- Color Definitions (RGB565 format) ---
#define COLOR_GREEN		0x07E0
#define COLOR_BLUE		0x001F
#define COLOR_RED		0xF800
#define COLOR_WHITE		0xFFFF
#define COLOR_CYAN		0x07FF
#define COLOR_ORANGE	0xFD20
#define COLOR_MAGENTA   0xF81F
#define COLOR_BG        0x0000 

// Initializes hardware peripherals and software subsystems
void init_all(){
    spi_init();                         // Initialize SPI communication (needed for the display)
	st7735_init();                      // Initialize the ST7735 TFT display driver
    input_init();                       // Initialize buttons and encoder inputs
	SoftwareTimerPool::initTimerPool(); // Setup the internal timer system 
    Soundboard::initSoundboard();       // Initialize the buzzer/speaker outputs
    Datalink::initDatalink();           // Setup UART communication for the multiplayer network
}

// Processes continuous background tasks
void tick_all(){
    input_tick();
    SoftwareTimerPool::tick();
	Soundboard::play();
    Messenger::commLoop();
}

int main(void) {
	// Enable global interrupts (required for timers and UART)
	sei();

	init_all();
	
	// Initialize the Finite State Machine 
	GameState current_state = STATE_MENU;
	bool state_just_changed = true;

	// Global player statistics
	extern	uint16_t numberOfShots;
	extern	uint16_t numberOfHits;
	extern	uint16_t totalDamage;

	// Main application loop
	while (1) {

		// Execute background tasks
		tick_all();

		// Handle hardware reset button -> Return to main menu and reset stats
		if (input_reset_button_rising() == true){
			current_state = STATE_MENU;
			state_just_changed = true;
			numberOfShots = 0;
			numberOfHits = 0;
			totalDamage = 0;
		}

		// State Machine Logic
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

			case STATE_ARSENAL: {
				// unused
				break;
			}

			case STATE_DEFEAT: {
				// Render defeat screen
				st7735_fill_screen(COLOR_RED);
				draw_char_buffer(10, 20, getName(getSelectedName()), 0, COLOR_WHITE, COLOR_BG);
				draw_char_buffer(10, 10, "Umrel(a)s!", 0, COLOR_WHITE, COLOR_BG);
				draw_ship(56, 50, COLOR_MAGENTA);

				// Print player stats
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
				// Render victory screen
				st7735_fill_screen(COLOR_GREEN);
				draw_char_buffer(10, 20, getName(getSelectedName()), 0, COLOR_WHITE, COLOR_BG);
				draw_char_buffer(10, 10, "Vyhral(a)s!", 0, COLOR_WHITE, COLOR_BG);
				draw_ship(56, 50, COLOR_CYAN);

				// Print player stats
				char buf[16];
				snprintf_P(buf, sizeof(buf), PSTR("Zasahy: %u"), numberOfHits);
				draw_char_buffer(10, 90, buf, 0, COLOR_WHITE, COLOR_BG);
				snprintf_P(buf, sizeof(buf), PSTR("Vystrely: %u"), numberOfShots);
				draw_char_buffer(10, 80, buf, 0, COLOR_WHITE, COLOR_BG);
				
				//snprintf(buf, sizeof(buf), "Poskozeni: %d", totalDamage);
				//draw_char_buffer(10, 100, buf, 15, COLOR_WHITE, COLOR_BG);
				
				Soundboard::playMelody(Soundboard::grandVictory);
				current_state = STATE_DEAD_END;
				break;
			}

			case STATE_FATAL_ERROR:
				draw_char_buffer(20, 50, "FATAL ERROR", 0, COLOR_WHITE, COLOR_BG);
				break;

			case STATE_DEAD_END:
				// Idle state, waiting for hardware reset
				break;
		}
	}
}