#include "hal/input.h"

// --- Encoder state ---
//static volatile int8_t enc_delta = 0;
uint8_t last_enc_A = 0;

// --- Button state (debounced edge detection) ---
typedef struct {
    uint8_t  constant_read_num;// number of constant raw readings
    uint8_t  last_raw;        // last raw read
    uint32_t read_timeout;      // when raw last changed
    bool     pressed;         // state of button
    bool     rising_edge_announced;    //true if function for rising edge reading was called     
} ButtonStruct;

static ButtonStruct enc_btn = { 0, 0, 0, false, false };
static ButtonStruct btn2    = { 0, 0, 0, false, false };

void input_init(void) {
    // Set PD2-PD5 as inputs with pull-ups
    DDRD  &= ~((1 << ENC_A_PIN) | (1 << ENC_B_PIN) |
               (1 << ENC_BTN_PIN) | (1 << BTN2_PIN));
    PORTD |=  (1 << ENC_A_PIN) | (1 << ENC_B_PIN) |
              (1 << ENC_BTN_PIN) | (1 << BTN2_PIN);

    // INT0 on falling edge of encoder A pin
    //EICRA |= (1 << ISC01);    // ISC01=1, ISC00=0 → falling edge
    //EIMSK |= (1 << INT0);     // enable INT0
}

// Encoder A falling edge — direction determined by B
/*ISR(INT0_vect) {
    if (PIND & (1 << ENC_B_PIN)) {
        enc_delta += (enc_delta <= +MAX_TICKS) ? 1 : 0;   // CCW
    } else {
        enc_delta -= (enc_delta >= -MAX_TICKS) ? 1 : 0;   // CW
    }
}*/

/*int8_t input_get_encoder_ticks(void) {
    int8_t result = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        result = enc_delta;
        enc_delta = 0;
    }
    if (result > 0){
        return 1;
    }
    else if (result < 0){
        return -1;
    }
    return result;
}*/

int8_t input_get_encoder_dir(){
    uint8_t current_enc_A = (PIND & (1 << PD2)) >> PD2;
    uint8_t ret_val = 0;

    if (current_enc_A != last_enc_A) {
        if (current_enc_A == 0) {
            uint8_t current_enc_B = (PIND & (1 << PD3)) >> PD3;
            
            if (current_enc_B != current_enc_A) {
                // Scroll up
                ret_val = -1;
                
            } else {
                // Scroll down
                ret_val = 1;
            }
        }
        last_enc_A = current_enc_A;
    }
    return ret_val;
}

// Generic debounce + edge detection helper
void button_check(ButtonStruct *btn, uint8_t pin) {
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
        if (btn->pressed == false){
            btn->rising_edge_announced = false;
        }
    }
}

void input_tick(){
    button_check(&enc_btn, ENC_BTN_PIN);
    button_check(&btn2, BTN2_PIN);
}

bool input_encoder_button_pressed(void) {
    return enc_btn.pressed;
}

bool input_fire_button_pressed(void) {
    return btn2.pressed;
}

bool input_encoder_button_rising(){
    if (enc_btn.pressed == true && enc_btn.rising_edge_announced == false){
        enc_btn.rising_edge_announced = true;
        return true;
    }
    return false;
}

bool input_fire_button_rising(){
    if (btn2.pressed == true && btn2.rising_edge_announced == false){
        btn2.rising_edge_announced = true;
        return true;
    }
    return false;
}