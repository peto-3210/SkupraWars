#ifndef INPUT_H
#define INPUT_H

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "hal/libraries.h"
#include "hardware_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pin assignments (PORTD)
#define ENC_A_PIN     PD2   // INT0
#define ENC_B_PIN     PD3   // INT1 (read-only here, no interrupt)
#define ENC_BTN_PIN   PD4   // encoder push button
#define BTN2_PIN      PD5   // second control button

// Debounce time in milliseconds
#define READ_DELAY_MS   25
#define CONSTANT_READ_NUM 2

//Max number of encoder ticks
#define MAX_TICKS 1

void input_init(void);

// Returns direction since last call, then resets to ENC_NONE
int8_t input_get_encoder_ticks(void);

// Returns current_button_state
bool input_encoder_button_pressed(void);
// Returns current_button_state
bool input_fire_button_pressed(void);

// Returns true if rising edge occured (only once)
bool input_encoder_button_rising(void);
// Returns true if rising edge occured (ony once)
bool input_fire_button_rising(void);

//Call periodically
void input_tick();

#ifdef __cplusplus
}
#endif

#endif