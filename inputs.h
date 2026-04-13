#ifndef INPUTS_H
#define INPUTS_H

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

// ================= ?????? =================
// ?? (Port A)
#define JOY_X_CH    PA6 
#define JOY_Y_CH    PA7    
#define JOY_SW_PIN  PA4// 

// ??? (Port D)
#define ENC_CLK_PIN PD4    
#define ENC_DT_PIN  PD5   
#define ENC_SW_PIN  PD6  

// ================= ?????? =================
typedef enum {
    EVENT_NONE = 0,
    
    // ????
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT,
    JOY_PRESS,  // ????
    JOY_HOLD,   // ???????
    
    // ?????
    ENC_CW,     // ?????
    ENC_CCW,    // ?????
    ENC_PRESS   // ?????
} InputEvent_t;

// ================= ???? =================
void Inputs_Init(void);
InputEvent_t Inputs_Scan(void);
uint16_t ADC_Read(uint8_t channel);

#endif