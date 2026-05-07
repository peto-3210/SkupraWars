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
#include "Communication/Messenger.hpp"
#include <hal/input.h>


int main0(void) {
	sei();

	st7735_init();
	st7735_fill_rect(5, 5, 2, 3, COLOR_RED);
	//_delay_ms(1000);
	SoftwareTimerPool::initTimerPool();
	Soundboard::initSoundboard();
	Datalink::initDatalink();
	input_init();

	st7735_fill_rect(5, 5, 2, 3, COLOR_GREEN);
	

	
	//_delay_ms(5000);

	//
	draw_char_buffer(20, 5, "Ema ma maso a vari", 0, COLOR_GREEN, COLOR_BG);

	if (Messenger::initTopology(3, 0x0004) == false){
		Soundboard::playSound(Soundboard::sfx_laser);
	}
	else {
		Soundboard::playSound(Soundboard::sfx_rocket);
	}
	

	while (1) {
		SoftwareTimerPool::tick();
		Soundboard::play();
	}
}
