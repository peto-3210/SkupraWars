#include "hal/libraries.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define UART_BUFFER_LEN 128

#define BAUD_RATE_CONSTANT_115200 8
#define BAUD_RATE_CONSTANT_19200 51
#define BAUD_RATE_CONSTANT_9600 103

/**
 * @brief Initializes UART
 * 
 * @param baud_rate_constant Defines speed of uart, can be found in manual
 */
void uart_begin(uint16_t baud_rate_constant);

/**
 * @brief Copies bytes to internal TX buffer
 * 
 * @param data Source data
 * @param length Number of bytes to send
 * 
 * @return Actual number of sent bytes
 */
uint8_t uart_send(uint8_t* data, uint8_t length);

/**
 * @brief Reads received bytes from internal RX buffer
 * 
 * @param data Buffer for read data
 * @param length Number of bytes to read
 * 
 * @return Actual number of read bytes
 */
uint8_t uart_recv(uint8_t* data, uint8_t length);

/**
 * @brief Whether the RX buffer is empty
 * 
 * @return True if RX buffer is empty, false otherwise
 */
bool uart_is_RX_empty();


#ifdef __cplusplus
    }
#endif