#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>  // using strcmp
#include <stdlib.h>  // atol char -> int

#include "twi.h"
#include "ssd1306.h"
#include "Si5351.h"
#include "inputs.h"
#include <avr/interrupt.h> // 需要用到 sei()
#include "uart.h"

uint32_t vfo_b_freq = 14000000UL;

// ================= UI Definitions =================

typedef enum {
    STATE_IDLE,
    STATE_SELECT,
    STATE_EDIT
} UI_State_t;

typedef enum { MODE_RX, MODE_TX } RadioMode_t;
typedef enum { CTRL_USER, CTRL_USB } ControlCtrl_t;

// Global Variables
UI_State_t ui_state = STATE_IDLE;
uint8_t cursor_pos = 0;       // 0:Mode, 1:Ctrl, 2:Freq
RadioMode_t radio_mode = MODE_RX;
ControlCtrl_t ctrl_source = CTRL_USER;

#define TXEN_PORT PORTB
#define TXEN_DDR  DDRB
#define TXEN_PIN  PB4

// Frequency Variable
uint32_t current_freq = 14000000UL; // Default 14 MHz

// Digit multipliers for frequency adjustment (10M, 1M, 100k, 10k, 1k, 100, 10, 1)
const uint32_t digit_multipliers[8] = {
    10000000UL, 1000000UL, 100000UL, 10000UL, 
    1000UL, 100UL, 10UL, 1UL
};

// Current digit index (0-7), default is index 1 (1M)
uint8_t freq_digit_idx = 1; 

// Mapping for string "XX.XXX.XXX" character positions
// For 14.000.000 -> index 0 is '1', index 1 is '4', index 3 is '0'...
const uint8_t char_pos_map[8] = {0, 1, 3, 4, 5, 7, 8, 9};

// Blink control variables
bool blink_state = true;
uint16_t blink_timer = 0;
// ================= Display Functions =================

uint8_t Get_Draw_Mode(uint8_t row_id) {
    if (ui_state == STATE_IDLE) return OLED_MODE_NORMAL;
    if (cursor_pos != row_id) return OLED_MODE_NORMAL;
    
    if (ui_state == STATE_SELECT) return OLED_MODE_INVERT; 
    
    // ???? Mode (row_id == 0) ??? EDIT ?????
    if (ui_state == STATE_EDIT && row_id == 0) {
        return blink_state ? OLED_MODE_INVERT : OLED_MODE_NORMAL;
    }
    
    return OLED_MODE_NORMAL;
}

// Draw frequency string on OLED
void Draw_Frequency(uint8_t y_page) {
    char buf[20];
    sprintf(buf, "%02lu.%03lu.%03lu Hz", 
            current_freq / 1000000, 
            (current_freq % 1000000) / 1000, 
            current_freq % 1000);

    // Current X offset
    uint8_t x_offset = 0;
    
    // Loop through each character and set highlight if necessary
    for (uint8_t i = 0; buf[i] != '\0'; i++) {
        uint8_t char_mode = OLED_MODE_NORMAL;
        
        // 1. In SELECT state, invert the whole Frequency line if selected
        if (ui_state == STATE_SELECT && cursor_pos == 1) {
            char_mode = OLED_MODE_INVERT;
        }
        // 2. In EDIT state, blink the currently selected digit
        else if (ui_state == STATE_EDIT && cursor_pos == 1) {
            if (i == char_pos_map[freq_digit_idx]) {
                char_mode = blink_state ? OLED_MODE_INVERT : OLED_MODE_NORMAL;
            }
        }
        
        // Each character is 6 pixels wide (6x8 font)
        OLED_ShowChar(SCREEN_R_ADDR, x_offset, y_page, buf[i], char_mode);
        x_offset += 6;
    }
}

// Update the right OLED screen
void Update_Right_Screen(void) {
    char buf[20];
    
    // 0: Mode
    sprintf(buf, "Mode: [%s]    ", (radio_mode == MODE_RX) ? "RX" : "TX");
    OLED_ShowString(SCREEN_R_ADDR, 0, 0, buf, Get_Draw_Mode(0));
    
    // 1: Ctrl (??????? NORMAL ??????????)
    sprintf(buf, "Ctrl: [%s]  ", (ctrl_source == CTRL_USER) ? "USER" : "USB ");
    OLED_ShowString(SCREEN_R_ADDR, 0, 2, buf, OLED_MODE_NORMAL);
    
    // 2: Freq Label 
    OLED_ShowString(SCREEN_R_ADDR, 0, 4, "Current Freq:     ", OLED_MODE_NORMAL);
    
    // 3: Frequency ?
    Draw_Frequency(6);
}

// Update the left OLED screen (Debug info)
void Update_Left_Screen(InputEvent_t event) {
    char buf[20];
    const char* state_str = "IDLE  ";
    if (ui_state == STATE_SELECT) state_str = "SELECT";
    else if (ui_state == STATE_EDIT) state_str = "EDIT  ";
    
    sprintf(buf, "State: %s", state_str);
    OLED_ShowString(SCREEN_L_ADDR, 1, 0, buf, OLED_MODE_NORMAL);// 0 ->1
    
    if (event != EVENT_NONE) {
        OLED_ShowString(SCREEN_L_ADDR, 1, 2, "                ", OLED_MODE_NORMAL);
        switch(event) {
            case JOY_UP:    OLED_ShowString(SCREEN_L_ADDR, 0, 3, "JOY: UP", OLED_MODE_INVERT); break;
            case JOY_DOWN:  OLED_ShowString(SCREEN_L_ADDR, 0, 3, "JOY: DOWN", OLED_MODE_INVERT); break;
            case JOY_LEFT:  OLED_ShowString(SCREEN_L_ADDR, 0, 3, "JOY: LEFT", OLED_MODE_INVERT); break;
            case JOY_RIGHT: OLED_ShowString(SCREEN_L_ADDR, 0, 3, "JOY: RIGHT", OLED_MODE_INVERT); break;
            case JOY_PRESS: OLED_ShowString(SCREEN_L_ADDR, 0, 3, "JOY: PRESS", OLED_MODE_INVERT); break;
            case ENC_CW:    OLED_ShowString(SCREEN_L_ADDR, 0, 3, "ENC: CW >>", OLED_MODE_INVERT); break;
            case ENC_CCW:   OLED_ShowString(SCREEN_L_ADDR, 0, 3, "ENC: << CCW", OLED_MODE_INVERT); break;
            case ENC_PRESS: OLED_ShowString(SCREEN_L_ADDR, 0, 3, "ENC: PRESS", OLED_MODE_INVERT); break;
            default: break;
        }
    }
}

void si5351_write_reg(uint8_t reg, uint8_t data) {
    twi_start();
    twi_MT_SLA_W(SI5351_ADDR);
    twi_MT_write(reg);
    twi_MT_write(data);
    twi_stop();
}

void Update_Si5351_Freq(uint32_t target_freq) {
    // 1. Calculate Divider
    uint32_t divider = 600000000UL / target_freq;
    while (divider % 4 != 0) divider++;
    while ((target_freq * divider) < 600000000UL) divider += 4;
    uint32_t f_vco = target_freq * divider;

    // 2. Calculate PLL Multiplier
    uint32_t aM = f_vco / 25000000UL;
    uint32_t remainder = f_vco % 25000000UL;
    uint32_t bM = remainder / 25; 
    uint32_t cM = 1000000;

    // 3. Setup PLL A
    setup_PLL(SI5351_PLL_A, aM, bM, cM);
    
    // 4. Setup CLK0 and CLK2 frequencies (Mapped to PORT0 and PORT2)
    setup_clock(SI5351_PLL_A, SI5351_PORT0, divider, 0, 1);
    setup_clock(SI5351_PLL_A, SI5351_PORT1, divider, 0, 1);

    // ================= Register Configurations =================
    // Registers 16 (CLK0), 17 (CLK1), 18 (CLK2) config
    // 0x4F: Power Up, Integer Mode, PLL A, Invert disabled, Multisynth enabled, 8mA drive strength
    // 0x80: Power Down (Disable clock)
    
    si5351_write_reg(16, 0x4F); // Enable CLK0 
    si5351_write_reg(17, 0x4F); // Power down CLK1 (Not used)
    si5351_write_reg(18, 0x80); // Enable CLK2
    
    // Register 3 enables/disables output (0 = enable, 1 = disable)
    // Enable CLK0 (Bit 0) and CLK2 (Bit 2) outputs
    // Binary: 1111 1100 -> Hex: 0xFA
    si5351_write_reg(3, 0xFC);
    // =========================================================

    // 5. Set 90 degree phase offset (1/4 period) for CLK2
    uint8_t phase_offset = (uint8_t)divider;
    si5351_write_reg(SI5351_REGISTER_165_CLK0_INITIAL_PHASE_OFFSET, phase_offset);
    
    // Write to REGISTER_167 to set CLK2 offset
    si5351_write_reg(SI5351_REGISTER_166_CLK1_INITIAL_PHASE_OFFSET, 0);

    // 6. Reset PLL A to apply phase changes
    si5351_write_reg(SI5351_REGISTER_177_PLL_RESET, SI5351_PLL_RESET_A);
}
// ================= Main Program =================
// ================= CAT ?????? =================
void Parse_CAT_Command(char* cmd) {
    char response[64];
    ctrl_source = CTRL_USB; 

    // 1. ?? TX ?? (??????)
    if (strncmp(cmd, "TX", 2) == 0) {
        if (cmd[2] == ';') { 
            // ??????
            sprintf(response, "TX%d;", (radio_mode == MODE_TX) ? 1 : 0);
            UART_SendString(response);
        } else if (cmd[2] == '0' || cmd[2] == '1') { 
            // ????
            if (cmd[2] == '1') {
                radio_mode = MODE_TX;
                TXEN_PORT &= ~(1 << TXEN_PIN); // ???????????
            } else {
                radio_mode = MODE_RX;
                TXEN_PORT |= (1 << TXEN_PIN);  // ???????????
            }
        }
    }
    
    // 2. ?? FA ?? (??????)
    else if (strncmp(cmd, "FA", 2) == 0) {
        if (cmd[2] == ';') { 
            // ?????????? 9 ???????
            sprintf(response, "FA%09lu;", current_freq);
            UART_SendString(response);
        } else { 
            // ????????????2????????
            uint32_t new_freq = atol(&cmd[2]);
            if (new_freq >= 1000000UL && new_freq <= 30000000UL) { // ????
                current_freq = new_freq;
                Update_Si5351_Freq(current_freq); // ???????????
            }
        }
    }
    
    // 3. ?? FB ?? (?????ICD ????????????)
    else if (strncmp(cmd, "FB", 2) == 0) {
        if (cmd[2] == ';') {
            sprintf(response, "FB%09lu;", vfo_b_freq);
            UART_SendString(response);
        } else {
            vfo_b_freq = atol(&cmd[2]);
        }
    }
    
    // 4. ?? IF ?? (????????????)
    else if (strncmp(cmd, "IF;", 3) == 0) {
        // ? ICD ?42?????? 28 ???????
        sprintf(response, "IF001%09lu+000000C00000;", current_freq);
        UART_SendString(response);
    }
    
    // 5. ?????? (???????????????? Yaesu ??)
    else if (strncmp(cmd, "ID;", 3) == 0) UART_SendString("ID0650;");
    else if (strncmp(cmd, "MD0;", 4) == 0) UART_SendString("MD0C;");
    else if (strncmp(cmd, "SH0;", 4) == 0) UART_SendString("SH0000;");
    else if (strncmp(cmd, "NA0;", 4) == 0) UART_SendString("NA00;");
    else if (strncmp(cmd, "AI", 2) == 0) {
        // AI ?????????????????? 0
        if (cmd[2] == ';') UART_SendString("AI0;"); 
    }
}

int main(void) {

    UART_Init();   // 初始化串口
    sei();         // 开启全局中断（极其重要，否则接收不到数据）
    UART_SendString("FLRTRX Radio Initialized!\r\n"); // 启动时给电脑发个问候语

    twi_init();
    Inputs_Init();
    
    //init pll
    si5351_init();
    Update_Si5351_Freq(current_freq);
    enable_clocks(true);  
    
    OLED_Init(SCREEN_L_ADDR); OLED_Clear(SCREEN_L_ADDR);
    OLED_Init(SCREEN_R_ADDR); OLED_Clear(SCREEN_R_ADDR);
    
    TXEN_DDR |= (1 << TXEN_PIN);  // Set PB4 as output
    TXEN_PORT |= (1 << TXEN_PIN); // Default to RX Mode (High = 3.3V)
    
    Update_Right_Screen();
    Update_Left_Screen(EVENT_NONE);
    
    uint8_t screen_refresh_timer = 0;

    while(1) {
        InputEvent_t event = Inputs_Scan();
        bool force_update = false;
        if (event != EVENT_NONE) {
            if (ctrl_source != CTRL_USER) {
                ctrl_source = CTRL_USER;
                force_update = true;
            }
        }
        if (cmd_ready) {
            // ????????????????
            Parse_CAT_Command((char*)rx_buffer);

            // ?????????????????????
            OLED_ShowString(SCREEN_L_ADDR, 0, 6, "                ", OLED_MODE_NORMAL);
            OLED_ShowString(SCREEN_L_ADDR, 0, 6, (char*)rx_buffer, OLED_MODE_INVERT);

            // ???????????????????
            UART_ResetBuffer();
            
            // ?????????????????????????
            force_update = true;
        }

        // ================= State Machine =================
        switch (ui_state) {
            
            case STATE_IDLE:
                if (event >= JOY_UP && event <= JOY_RIGHT) {
                    ui_state = STATE_SELECT;
                    cursor_pos = 0; // Default to Mode
                    force_update = true;
                }
                break;
                
            case STATE_SELECT:
                if (event == JOY_UP) {
                    if (cursor_pos > 0) cursor_pos--;
                    force_update = true;
                } 
                else if (event == JOY_DOWN) {
                    if (cursor_pos < 1) cursor_pos++; // Max index is 2 (Freq)
                    force_update = true;
                }
                else if (event == JOY_PRESS) {
                    ui_state = STATE_EDIT;
                    freq_digit_idx = 1; // Reset to default digit when entering EDIT mode
                    blink_state = true;
                    blink_timer = 0;
                    force_update = true;
                }
                // Encoder is ignored in Select state
                break;
                
            case STATE_EDIT:
                // --- A. Edit Mode/Ctrl (0, 1) ---
                if (cursor_pos == 0) {
                    if (event == ENC_CW || event == ENC_CCW) {
                        if (cursor_pos == 0) {
                            // Toggle state
                            radio_mode = (radio_mode == MODE_RX) ? MODE_TX : MODE_RX;

                            // Update Hardware Pin
                            if (radio_mode == MODE_RX) {
                                TXEN_PORT |= (1 << TXEN_PIN);  // Set High (3.3V) for RX
                            } else {
                                TXEN_PORT &= ~(1 << TXEN_PIN); // Set Low (0V) for TX
                            }
                        }
                        //if (cursor_pos == 1) ctrl_source = (ctrl_source == CTRL_USER) ? CTRL_USB : CTRL_USER;
                        
                        blink_state = true; blink_timer = 0; force_update = true;
                    }
                    // End of Mode/Ctrl edit
                }
                
                // --- B. Edit Frequency (2) ---
                else if (cursor_pos == 1) {
                    // Move cursor left/right
                    if (event == JOY_LEFT) {
                        if (freq_digit_idx > 0) freq_digit_idx--;
                        blink_state = true; blink_timer = 0; force_update = true;
                    }
                    else if (event == JOY_RIGHT) {
                        if (freq_digit_idx < 7) freq_digit_idx++;
                        blink_state = true; blink_timer = 0; force_update = true;
                    }
                    // Adjust frequency value
                    else if (event == ENC_CW || event == ENC_CCW) {
                        uint32_t step_val = digit_multipliers[freq_digit_idx];
                        
                        if (event == ENC_CW) {
                            if (current_freq + step_val <= 16000000UL) current_freq += step_val;
                            else current_freq = 16000000UL; // Upper limit
                        } else {
                            if (current_freq >= 8000000UL + step_val) current_freq -= step_val;
                            else current_freq = 8000000UL; // Lower limit
                        }
                        Update_Si5351_Freq(current_freq);
                        blink_state = true; blink_timer = 0; force_update = true;
                    }
                    // End of Frequency edit
                }
                
                // --- C. Exit Edit state ---
                if (event == ENC_PRESS) {
                    ui_state = STATE_SELECT;
                    force_update = true;
                }
                break;
        }

        // ================= Blink Timer (approx 500ms) =================
        if (ui_state == STATE_EDIT) {
            blink_timer++;
            if (blink_timer >= 80) {
                blink_state = !blink_state;
                blink_timer = 0;
                force_update = true;
            }
        }

        // ================= Screen Refresh =================
        screen_refresh_timer++;
        if (force_update || event != EVENT_NONE || screen_refresh_timer >= 50) {
            Update_Right_Screen();
            if (event != EVENT_NONE) Update_Left_Screen(event);
            screen_refresh_timer = 0;
        }

        _delay_ms(2); // Base delay for loop timing and debouncing
    }
}