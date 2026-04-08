#include "hal/uart.h"

#define UART_RX_DONE_INT_ON() {UCSR0B |= 0b10000000;}
#define UART_RX_DONE_INT_OFF() {UCSR0B &= ~0b10000000;}

#define UART_TX_DONE_INT_ON() {UCSR0B |= 0b01000000;}
#define UART_TX_DONE_INT_OFF() {UCSR0B &= ~0b01000000;}

#define UART_TX_UDRE_INT_ON() {UCSR0B |= 0b00100000;}
#define UART_TX_UDRE_INT_OFF() {UCSR0B &= ~0b00100000;}

/**
 * @brief Structure for handling UART communication
 * 
 * @param data_buffer Stores received/sent bytes
 * @param iterator Stores position of FIRST FREE BYTE after data in buffer
 */
typedef struct {
    uint8_t data_buffer[UART_BUFFER_LEN];
    uint8_t iterator;

} transmission_handler;


volatile transmission_handler tx_handler = {};
volatile transmission_handler rx_handler = {};

void uart_begin(uint16_t baud_rate_constant) {
    UCSR0A = 0; //Nothing interesting to set here
    UCSR0B = 0b00011000; //Enable transmitter and receiver
    UCSR0C = 0b00100110; //Set even parity, 8 data bits and 1 stop bit

    UBRR0L = baud_rate_constant & 0xff;
    UBRR0H = baud_rate_constant >> 8;
    UART_RX_DONE_INT_ON()
}

uint8_t uart_recv(uint8_t* data, uint8_t length){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        //Not enough bytes in buffer
        if (length > rx_handler.iterator){
            length = rx_handler.iterator;
        }

        for (uint8_t i = 0; i < length; ++i){
            data[i] = rx_handler.data_buffer[i];
        }

        //If not all bytes were read, move them to clean up space
        if (rx_handler.iterator > length){
            memmove((void*)rx_handler.data_buffer, (const void*)rx_handler.data_buffer + length, rx_handler.iterator - length);
        }
        rx_handler.iterator -= length;
    }
    return length;
}

ISR(USART_RX_vect){
    if (rx_handler.iterator <= UART_BUFFER_LEN){
        rx_handler.data_buffer[rx_handler.iterator++] = UDR0;
        
    }
    else {
        (void)UDR0;  // explicitly discard
    }
}

uint8_t uart_send(uint8_t* data, uint8_t length){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        //Not enough space in buffer
        if (length > UART_BUFFER_LEN - tx_handler.iterator){
            length = UART_BUFFER_LEN - tx_handler.iterator;
        }

        for (uint8_t i = 0; i < length; ++i){
            tx_handler.data_buffer[tx_handler.iterator + i] = data[i];
        }
        tx_handler.iterator += length;
        UART_TX_UDRE_INT_ON()
    }
    return length;
}


ISR(USART_UDRE_vect){
    if (tx_handler.iterator > 0){
        UDR0 = tx_handler.data_buffer[--tx_handler.iterator];
    }
    else {
        UART_TX_UDRE_INT_OFF()
    }
}




