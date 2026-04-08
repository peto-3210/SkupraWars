#include "hal/hardware_timer.h"

volatile uint32_t current_microseconds = 0;
uint8_t period_us = 0;

void hardware_timer_init(uint8_t period){
    current_microseconds = 0;
    TCCR0A = 0b01000010; //Set timer to CTC mode + enable output on pin
    TCCR0B = 0b00000010; //Set time source to internal clock divided by 8
    TCNT0 = 0; //Set timer register value to 0
    period_us = period;
    OCR0A = period_us * 2 - 1; //Set value for time compare (why -1? - overflow (from max to 0) count as a tick too)
    TIMSK0 = 0b00000010;  //Allow interrupt from compare match A
}

uint32_t micros(){
    uint32_t value;
    //To disable interrupts while reading value
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        value = current_microseconds;
    }
    return value;
}


ISR(TIMER0_COMPA_vect){
    current_microseconds += period_us;
}

