#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include "hal/spi.h"
#include "hal/st7735.h"
#include "Datalink.hpp"
#include "SoftwareTimer.hpp"
#include "Soundboard.hpp"

int main(void)
{
	// ==========================================
	// INICIALIZACE VŠEHO
	// ==========================================
	sei(); // Povolení pøerušení (velmi dùležité pro timer/sound!)
	
	// Piny
	DDRB = 0b00100010;
	DDRD = 0b01000000;
	PORTB = 0b00000000;
	
	// UART, SPI a Displej
	uart_begin(8);
	spi_init();
	st7735_init();
	st7735_fill_screen(0x001F); // 0x001F - blue; 0xFFFF - red

	// Enkodér - vstupy a pull-up rezistory
	DDRD &= ~((1 << PD2) | (1 << PD3) | (1 << PD4));
	PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD4);

	// Timery a Soundboard
	SoftwareTimerPool::initTimerPool(10);
	SoftwareTimer* timer1 = SoftwareTimerPool::acquireTimer();
	SoftwareTimer* btnTimer = SoftwareTimerPool::acquireTimer();
	Soundboard::initSoundboard(false);
	Soundboard::playMelody(Soundboard::imperialMarch);
	
	timer1->startTimerUs(500000);

	// Promìnné pro enkodér
	uint8_t x = 64;
	uint8_t y = 80;
	bool move_horizontal = true;
	st7735_draw_pixel(x, y, 0xF800);
	uint8_t last_A = (PIND & (1 << PD2)) >> PD2;

	// Promìnné pro soundboard
	bool toggle = false;
	bool toggle1 = false;
	uint8_t buffer[1] = {0};
		
	// Pøehledný seznam stavù pro naše tlaèítko
	typedef enum {
		BTN_IDLE,             // Tlaèítko je v klidu
		BTN_WAIT_DEBOUNCE,    // Zaznamenali jsme stisk, èekáme 50ms na potvrzení
		BTN_WAIT_RELEASE,     // Tlaèítko je držené, èekáme až ho pustíš
		BTN_RELEASE_DEBOUNCE  // Tlaèítko puštìno, èekáme 50ms na uklidnìní kontaktù
	} ButtonState;

	ButtonState btn_state = BTN_IDLE; // Výchozí stav

	// ==========================================
	// HLAVNÍ SUPER-SMYÈKA 
	// ==========================================
	while (1) {

		SoftwareTimerPool::tick();
		Soundboard::play();
		
		if (timer1->isDone() == true) {
			toggle = !toggle;
			timer1->startTimerUs(500000);
		}
		
		if ((uart_recv(buffer, 1)) == 1 && buffer[0] == 0xcc) {
			toggle1 = !toggle1;
			buffer[0] = 0;
			uint16_t a = 0xaabb;
			uart_send((uint8_t*)&a, 2);
		}
		PORTB = (((int)toggle) << 5) | (((int)toggle1) << 4);

		// --- ÈTENÍ TLAÈÍTKA ---
		switch (btn_state) {
			case BTN_IDLE:
			// Pokud je stisknuto, spustíme timer a pøepneme stav
			if (!(PIND & (1 << PD4))) {
				btnTimer->startTimerUs(50000);
				btn_state = BTN_WAIT_DEBOUNCE;
			}
			break;

			case BTN_WAIT_DEBOUNCE:
			// Èekáme, až timer dojede na nulu
			if (btnTimer->isDone()) {
				// Je po 50ms tlaèítko stále stisknuté? (není to jen šum?)
				if (!(PIND & (1 << PD4))) {
					move_horizontal = !move_horizontal; // AKCE! Pøepneme smìr
					btn_state = BTN_WAIT_RELEASE;       // Jdeme èekat na puštìní
					} else {
					btn_state = BTN_IDLE;               // Byl to falešný poplach
				}
			}
			break;

			case BTN_WAIT_RELEASE:
			// Èekáme na puštìní (logická 1)
			if (PIND & (1 << PD4)) {
				btnTimer->startTimerUs(50000);
				btn_state = BTN_RELEASE_DEBOUNCE;
			}
			break;

			case BTN_RELEASE_DEBOUNCE:
			// Èekáme, až timer dojede na nulu, než dovolíme další stisk
			if (btnTimer->isDone()) {
				btn_state = BTN_IDLE;
			}
			break;
		}

		// --- ÈTENÍ ENKODÉRU ---
		uint8_t current_A = (PIND & (1 << PD2)) >> PD2;

		if (current_A != last_A) {
			if (current_A == 0) {
				uint8_t current_B = (PIND & (1 << PD3)) >> PD3;

				st7735_draw_pixel(x, y, 0xFFFF);

				if (current_B != current_A) {
					if (move_horizontal) { if (x < 127) x--; }
					else                 { if (y < 159) y--; }
					} else {
					if (move_horizontal) { if (x > 0) x++; }
					else                 { if (y > 0) y++; }
				}

				st7735_draw_pixel(x, y, 0xF800);
			}
			last_A = current_A;
		}
	}

	return 0;
}