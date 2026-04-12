#define F_CPU 16000000UL // Frekvence pro delay (ujisti se, že odpovídá tvému nastavení)
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include "hal/spi.h"
#include "hal/st7735.h"

int main(void) {
	// 1. INICIALIZACE DISPLEJE A SPI (Ponechávám tvùj pùvodní základ)
	spi_init();
	st7735_init();
	st7735_fill_screen(0xFFFF); // Vyplníme bíle (0xFFFF je bílá)

	// 2. INICIALIZACE PINÙ PRO ENKODÉR (Port D)
	// Nastavíme PD2 (A), PD3 (B) a PD4 (Tlaèítko) jako VSTUPY (zápis nuly)
	DDRD &= ~((1 << PD2) | (1 << PD3) | (1 << PD4));
	
	// Zapneme interní PULL-UP rezistory (zápis jednièky na vstupní pin)
	// Tím zajistíme, že když nejsou spojené se zemí, bude na nich logická 1
	PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD4);

	// 3. PROMÌNNÉ PRO STAV A KRESLENÍ
	uint8_t x = 64; // Zaèínáme zhruba uprostøed (pro 128x160 displej)
	uint8_t y = 80;
	bool move_horizontal = true; // True = doleva/doprava, False = nahoru/dolù
	
	// Nakreslíme výchozí pixel (èervený)
	st7735_draw_pixel(x, y, 0xF800);

	// Promìnná pro uchování minulého stavu pinu A
	uint8_t last_A = (PIND & (1 << PD2)) >> PD2;

	while (1) {
		// --- A. ÈTENÍ TLAÈÍTKA (Zmìna smìru posuvu) ---
		// Protože máme pull-up, stisknutí tlaèítka spojí pin se zemí (logická 0)
		if (!(PIND & (1 << PD4))) {
			_delay_ms(50); // Debounce (ošetøení zákmytù kontaktù)
			if (!(PIND & (1 << PD4))) { // Znovu ovìøíme, jestli je opravdu stisknuto
				move_horizontal = !move_horizontal; // Pøepneme smìr
				
				// Èekáme, dokud tlaèítko nepustíš, aby se to nepøepínalo stokrát za sekundu
				while (!(PIND & (1 << PD4)));
				_delay_ms(50); // Debounce pøi puštìní
			}
		}

		// --- B. ÈTENÍ ENKODÉRU (Pohyb) ---
		uint8_t current_A = (PIND & (1 << PD2)) >> PD2; // Aktuální stav pinu A

		// Pokud se stav A zmìnil a zrovna klesl na nulu (krok enkodéru)
		if (current_A != last_A) {
			if (current_A == 0) {
				uint8_t current_B = (PIND & (1 << PD3)) >> PD3; // Pøeèteme pin B

				// 1. Vymažeme starý pixel (pøekreslíme ho bíle)
				st7735_draw_pixel(x, y, 0xFFFF);

				// 2. Vypoèítáme nové souøadnice podle smìru otáèení
				if (current_B != current_A) {
					// Toèíme jedním smìrem (+)
					if (move_horizontal) { if (x < 127) x--; }
					else                 { if (y < 159) y--; }
					} else {
					// Toèíme druhým smìrem (-)
					if (move_horizontal) { if (x > 0) x++; }
					else                 { if (y > 0) y++; }
				}

				// 3. Vykreslíme pixel na nové pozici (èervenì)
				st7735_draw_pixel(x, y, 0xF800);
			}
			last_A = current_A; // Uložíme si stav pro další kolo
		}
	}
}