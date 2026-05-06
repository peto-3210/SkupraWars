#include "hal/input.h"

// --- Encoder state ---
static volatile int8_t enc_delta = 0;

// --- Button state (debounced edge detection) ---
typedef struct {
    uint8_t  constant_read_num;// number of constant raw readings
    uint8_t  last_raw;        // last raw read
    uint32_t read_timeout;      // when raw last changed
    bool     pressed;         // state of button
} ButtonState;

static ButtonState enc_btn = { 0, 0, 0, false };
static ButtonState btn2    = { 0, 0, 0, false };

void input_init(void) {
    // Set PD2-PD5 as inputs with pull-ups
    DDRD  &= ~((1 << ENC_A_PIN) | (1 << ENC_B_PIN) |
               (1 << ENC_BTN_PIN) | (1 << BTN2_PIN));
    PORTD |=  (1 << ENC_A_PIN) | (1 << ENC_B_PIN) |
              (1 << ENC_BTN_PIN) | (1 << BTN2_PIN);

    // INT0 on falling edge of encoder A pin
    EICRA |= (1 << ISC01);    // ISC01=1, ISC00=0 → falling edge
    EIMSK |= (1 << INT0);     // enable INT0
}

// Encoder A falling edge — direction determined by B
ISR(INT0_vect) {
    if (PIND & (1 << ENC_B_PIN)) {
        enc_delta -= (enc_delta >= -MAX_TICKS) ? 1 : 0;   // CCW
    } else {
        enc_delta += (enc_delta <= +MAX_TICKS) ? 1 : 0;   // CW
    }
}

int8_t input_get_encoder_ticks(void) {
    int8_t result = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        result = enc_delta;
        enc_delta = 0;
    }
    return result;
}

// Generic debounce + edge detection helper
void button_check(ButtonState *btn, uint8_t pin) {
    uint8_t raw = (PIND & (1 << pin)) ? 1 : 0;
    uint32_t now = micros();

    if (raw != btn->last_raw) {
        btn->last_raw = raw;
        btn->read_timeout = now;
        btn->constant_read_num = 0;
    }
    else if ((now - btn->read_timeout) >= READ_DELAY_MS * 1000UL) {
        btn->constant_read_num++;
        btn->read_timeout = now;
    }

    if (btn->constant_read_num >= CONSTANT_READ_NUM) {
        btn->constant_read_num = 0;
        btn->pressed = !raw;
    }
}

void input_tick(){
    button_check(&enc_btn, ENC_BTN_PIN);
    button_check(&btn2, BTN2_PIN);
}

bool input_encoder_button_pressed(void) {
    return enc_btn.pressed;
}

bool input_button2_pressed(void) {
    return btn2.pressed;
}