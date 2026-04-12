#include "hal/hardware_timer.h"

#ifndef HARDWARE_TIMER
#define HARDWARE_TIMER

volatile uint32_t current_microseconds = 0;
uint8_t period_us = 0;

void hardware_timer_init(uint8_t period){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        current_microseconds = 0;
        TCCR0A = 0b00000010; //Set timer to CTC mode + enable output on pin //0b01000010
        TCCR0B = 0b00000010; //Set time source to internal clock divided by 8
        TCNT0 = 0; //Set timer register value to 0
        period_us = period;
        OCR0A = period_us * 2 - 1; //Set value for time compare (why -1? - overflow (from max to 0) count as a tick too)
        TIMSK0 = 0b00000010;  //Allow interrupt from compare match A
    }
}

void hardware_pwm_init(){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        DDRB |= 0b00000010; //Enables OC1A pin
        TCNT1 = 0;
        OCR1A = 0;
        TIMSK1 = 0;
    }
}

void hardware_pwm_set(uint32_t freq){
    uint32_t ocr = (16000000UL / (256UL * freq * 2)) - 1;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        TCCR1B = 0b00001000;   // Stop clock, keep CTC mode
        TCNT1  = 0;
        OCR1A  = ocr & 0xffff;

        //TCCR1A = 0b10000000;   // COM1A = 10 (clear on match) temporarily
        //TCCR1C = (1 << FOC1A); // Force compare - drive OC1A LOW, sets flip-flop to 0

        TCCR1A = 0b01000000;   // Toggle OC1A on compare match
        TCCR1B = 0b00001100;   // CTC (WGM12) + prescaler 256
    }
}

void hardware_pwm_reset(){
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        TCCR1B &= ~0b00000111; //Stops clock
        TCCR1A &= ~((1 << COM1A1) | (1 << COM1A0)); // Disconnect OC1A
        PORTB &= ~0b00000010; //Sets output to low
    }
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

#endif