#include "hal/spi.h"

void spi_init(void) {
    // Nastavení pinů pro SPI jako výstupní:
    // PB2 (CS), PB3 (MOSI), PB5 (SCK)
    DDRB |= (1 << PB2) | (1 << PB3) | (1 << PB5);
    
    // MISO (PB4) je vstupní (pro jistotu, i když výchozí stav to zajišťuje)
    DDRB &= ~(1 << PB4);

    // Abychom se ujistili, že Slave (displej) není vybrán, nastavíme CS na HIGH
    PORTB |= (1 << PB2);

    // Nastavení SPI kontrolního registru (SPCR):
    // SPE = SPI Enable 
    // MSTR = Master (jsme pánem komunikace)
    // SPR0 a SPR1 = 0 (tím společně s SPI2X=1 nastavíme max. rychlost)
    SPCR = (1 << SPE) | (1 << MSTR);
    
    // Zapnutí dvojnásobné rychlosti hodin pro SPI (f_osc / 2)
    SPSR = (1 << SPI2X);
}

uint8_t spi_transfer(uint8_t data) {
    // Vložení dat do registru odstartuje odesílání
    SPDR = data;
    
    // Čekání na dokončení přenosu (kontrolujeme vlajku SPIF)
    while (!(SPSR & (1 << SPIF)));
    
    // Vrácení přijatých dat (displej sice většinou nic nevrací, ale standard to vyžaduje)
    return SPDR;
}