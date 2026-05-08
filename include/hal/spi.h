#ifndef SPI_H
#define SPI_H

#include "libraries.h" 

#define SPI_MOSI PB3
#define SPI_MISO_UNUSED PB4
#define SPI_SCK PB5
#define SPI_CS PB2

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief Init SPI on ATmega328P (Master)
 */
void spi_init(void);

/**
 * @brief Transfers and recieve 1 byte
 * * @param data Transfered byte
 * @return Recieved byte
 */
uint8_t spi_transfer(uint8_t data);

#ifdef __cplusplus
    }
#endif

#endif // SPI_H