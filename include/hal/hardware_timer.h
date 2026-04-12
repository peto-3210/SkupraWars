#include "hal/libraries.h"

#ifdef __cplusplus
    extern "C" {
#endif

/*
This library serves as a hardware timer interface.
It initializes hardware timer and provides exact timestamps when requested.
*/

/**
 * @brief Initializes hardware-based timer
 * 
 * @param period Period of one tick (in microseconds). Due to 8-bit timer,
 * max value is 127.
 * Hardware works at 16MHz with prescaler 8, therefore single hardware
 * tick lasts 2 microseconds.
 */
void hardware_timer_init(uint8_t period);

/**
 * @brief Iitializes hardware PWM timer
 */
void hardware_pwm_init();

/**
 * @brief Sets timer with specified frequency
 * 
 * @param freq Desired frequency
 */
void hardware_pwm_set(uint32_t freq);

/**
 * @brief Stops pwm timer
 */
void hardware_pwm_reset();

/**
 * @brief Retrieve number of microseconds from internal counter
 * 
 * @return Number of microseconds
 */
uint32_t micros();

#ifdef __cplusplus
    }
#endif