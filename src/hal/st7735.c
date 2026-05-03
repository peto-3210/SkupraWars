#include "hal/st7735.h"
#include "hal/spi.h"
#include <util/delay.h> // Potřebujeme pro čekání při inicializaci

void st7735_write_command(uint8_t cmd) {
    ST7735_DC_CMD();        // Říkáme displeji: "Tohle je příkaz!"
    ST7735_CS_LOW();        // Vybereme displej (začátek komunikace)
    spi_transfer(cmd);      // Odešleme přes SPI
    ST7735_CS_HIGH();       // Ukončíme komunikaci
}

void st7735_write_data(uint8_t data) {
    ST7735_DC_DATA();       // Říkáme displeji: "Tohle jsou data!"
    ST7735_CS_LOW();
    spi_transfer(data);
    ST7735_CS_HIGH();
}

void st7735_init(void) {
    // Nastavení pinů DC (PD0) a RST (PB0) jako výstupy
	DDRB |= (1 << PB0);
	DDRD |= (1 << PD0);
    
    // Hardwarový reset
    ST7735_RST_HIGH();
    _delay_ms(10);
    ST7735_RST_LOW();
    _delay_ms(50);
    ST7735_RST_HIGH();
    _delay_ms(120);
    
    // Softwarový reset (Command 0x01)
    st7735_write_command(0x01);
    _delay_ms(150);
    
    // Konec režimu spánku (Command 0x11)
    st7735_write_command(0x11);
    _delay_ms(120);

    // Nastavení formátu barev na 16-bit / pixel (Command 0x3A)
    st7735_write_command(0x3A);
    st7735_write_data(0x05); // 0x05 = 16-bit
    
    // Orientace displeje 
    st7735_write_command(0x36);
    st7735_write_data(0xC0); // 0xC0 otočí displej o 180°
    
    // Zapnutí displeje (Command 0x29)
    st7735_write_command(0x29);
}

void st7735_fill_screen(uint16_t color) {
    // Příkaz pro nastavení okna vykreslování (celá obrazovka 128x160)
    st7735_write_command(0x2A); // Nastavení sloupců
    st7735_write_data(0x00); st7735_write_data(0x00); // Start: 0
    st7735_write_data(0x00); st7735_write_data(127);  // Konec: 127
    
    st7735_write_command(0x2B); // Nastavení řádků
    st7735_write_data(0x00); st7735_write_data(0x00); // Start: 0
    st7735_write_data(0x00); st7735_write_data(159);  // Konec: 159
    
    st7735_write_command(0x2C); // Příkaz pro zápis do paměti RAM (kreslení)
    
    // Rozdělíme 16-bit barvu na dva 8-bitové bajty
    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;
    
    ST7735_DC_DATA();
    ST7735_CS_LOW();
    
    // Vyplníme všech 128 * 160 pixelů
    for (uint32_t i = 0; i < (128 * 160); i++) {
        spi_transfer(color_high);
        spi_transfer(color_low);
    }
    
    ST7735_CS_HIGH();
}

void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
	// 1. Ochrana proti vykreslení mimo obrazovku
	if (x >= 128 || y >= 160) return;
	if (x + w > 128) w = 128 - x;
	if (y + h > 160) h = 160 - y;

	// 2. Nastavení okna pro sloupce (X až X + šířka - 1)
	st7735_write_command(0x2A);
	st7735_write_data(0x00); st7735_write_data(x);
	st7735_write_data(0x00); st7735_write_data(x + w - 1);
	
	// 3. Nastavení okna pro řádky (Y až Y + výška - 1)
	st7735_write_command(0x2B);
	st7735_write_data(0x00); st7735_write_data(y);
	st7735_write_data(0x00); st7735_write_data(y + h - 1);
	
	// 4. Příkaz pro zápis barev
	st7735_write_command(0x2C);
	
	// 5. Rozdělení barvy
	uint8_t color_high = color >> 8;
	uint8_t color_low = color & 0xFF;
	
	ST7735_DC_DATA();
	ST7735_CS_LOW();
	
	// 6. Rychlé odeslání všech pixelů v daném obdélníku
	uint16_t total_pixels = w * h;
	for (uint16_t i = 0; i < total_pixels; i++) {
		spi_transfer(color_high);
		spi_transfer(color_low);
	}
	
	ST7735_CS_HIGH();
}

void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color) {
	// 1. Nastavíme okno pro kreslení jen na ten jeden konkrétní pixel (Start = Konec)
	st7735_write_command(0x2A);
	st7735_write_data(0x00); st7735_write_data(x);
	st7735_write_data(0x00); st7735_write_data(x);
	
	st7735_write_command(0x2B);
	st7735_write_data(0x00); st7735_write_data(y);
	st7735_write_data(0x00); st7735_write_data(y);
	
	// 2. Pošleme příkaz pro zápis barvy
	st7735_write_command(0x2C);
	
	// 3. Pošleme samotnou barvu
	ST7735_DC_DATA();
	ST7735_CS_LOW();
	spi_transfer(color >> 8);
	spi_transfer(color & 0xFF);
	ST7735_CS_HIGH();
}