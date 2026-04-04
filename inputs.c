#ifndef F_CPU
#define F_CPU 1000000UL 
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "inputs.h"

static uint8_t enc_prev_state = 0x03;
static int8_t  enc_counter = 0;
// ????????
static int8_t enc_accum = 0; 

void ADC_Init() {
    ADMUX |= (1 << REFS0);
    ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_Read(uint8_t channel) {
    channel &= 0x07;
    ADMUX = (ADMUX & 0xF0) | channel;
    _delay_us(20); 
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

void Inputs_Init(void) {
    ADC_Init(); 
    
    // ???? PB3
    DDRA &= ~(1 << JOY_SW_PIN);
    PORTA |= (1 << JOY_SW_PIN);
    
// ?????????????????? (??? DDRC/PORTC ?????????)
    DDRC &= ~((1 << ENC_CLK_PIN) | (1 << ENC_DT_PIN) | (1 << ENC_SW_PIN));
    PORTD |= (1 << ENC_CLK_PIN) | (1 << ENC_DT_PIN) | (1 << ENC_SW_PIN);
    
    // ????????????????????? 0x00~0x03 ???
    enc_prev_state = 0;
    if (PIND & (1 << ENC_CLK_PIN)) enc_prev_state |= 0x01;
    if (PIND & (1 << ENC_DT_PIN))  enc_prev_state |= 0x02;
    
    enc_prev_state = (PIND >> 2) & 0x03;
}

InputEvent_t Inputs_Scan(void) {
    
    uint8_t enc_curr_state = 0;
    if (PIND & (1 << ENC_CLK_PIN)) enc_curr_state |= 0x01;
    if (PIND & (1 << ENC_DT_PIN))  enc_curr_state |= 0x02;
    
    if (enc_curr_state != enc_prev_state) {
        // ???????????????(+1)????(-1)???????????0
        static const int8_t enc_table[16] = {
            0, -1,  1,  0,
            1,  0,  0, -1,
           -1,  0,  0,  1,
            0,  1, -1,  0
        };
        
        uint8_t transition = (enc_prev_state << 2) | enc_curr_state;
        enc_prev_state = enc_curr_state;
        
        // ?????????
        enc_counter += enc_table[transition];
        
        // 0x03 (??? 11) ?????????????????
        // ???????????????????
        if (enc_curr_state == 0x03) {
            int8_t count = enc_counter;
            enc_counter = 0; // ????????????????
            
            // ???1???? 4 ????? (count=4 ? -4)?
            // ???? 2????????????????????? 100% ?????
            if (count >= 2) return ENC_CCW;
            if (count <= -2) return ENC_CW;
        }
    }

    // --- 2. ????? ---
    if (!(PIND & (1 << ENC_SW_PIN))) {
        _delay_ms(20); 
        if (!(PIND & (1 << ENC_SW_PIN))) return ENC_PRESS;
    }

    // ==========================================
    // 3. ???? (PB3)
    // ==========================================
    if (!(PINA & (1 << JOY_SW_PIN))) {
        _delay_ms(20);
        if (!(PINA & (1 << JOY_SW_PIN))) return JOY_PRESS;
    }

    // ==========================================
    // 4. ????
    // ==========================================
    uint16_t x_val = ADC_Read(JOY_X_CH);
    uint16_t y_val = ADC_Read(JOY_Y_CH);
    
//    if (x_val < 100) return JOY_LEFT;// for formal design
//    if (x_val > 900) return JOY_RIGHT;
//    if (y_val < 100) return JOY_UP;
//    if (y_val > 900) return JOY_DOWN;
    
    if (x_val < 100) return JOY_RIGHT;// for temp prototype
    if (x_val > 900) return JOY_LEFT;
    if (y_val < 100) return JOY_DOWN;
    if (y_val > 900) return JOY_UP;
    
    return EVENT_NONE;
}