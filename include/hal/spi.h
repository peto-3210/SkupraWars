#ifndef SPI_H
#define SPI_H

#include "libraries.h" // Obsahuje <avr/io.h> a další věci z vašeho projektu

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief Inicializuje hardwarové SPI na ATmega328P (Master mód)
 */
void spi_init(void);

/**
 * @brief Odešle a zároveň přijme jeden bajt přes SPI
 * * @param data Bajt k odeslání
 * @return Přijatý bajt
 */
uint8_t spi_transfer(uint8_t data);

#ifdef __cplusplus
    }
#endif

#endif // SPI_H