#include "hal/spi.h"

void spi_init(void) {
    // Configure SPI pins as output (CS, MOSI, SCK)
    DDRB |= (1 << SPI_CS) | (1 << SPI_MOSI) | (1 << SPI_SCK);

    // Set CS high to ensure the slave is initially deselected
    PORTB |= (1 << SPI_CS);

    // Enable SPI in Master mode
    // Note: SPR0 and SPR1 remain 0 for maximum speed setup
    SPCR = (1 << SPE) | (1 << MSTR);
    
    // Enable double speed mode (f_osc / 2)
    SPSR = (1 << SPI2X);
}

uint8_t spi_transfer(uint8_t data) {
    // Start transmission by writing to the SPI Data Register
    SPDR = data;
    
    // Wait for the transmission to complete (poll the SPIF flag)
    while (!(SPSR & (1 << SPIF)));
    
    // Return received data (required by the SPI protocol standard)
    return SPDR;
}