#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

// ???? 64 ????????
#define RX_BUF_SIZE 64
volatile char rx_ring[RX_BUF_SIZE];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

void UART_Init(void) {
    UBRR1H = 0;
    UBRR1L = 12;
    
    // ???????1????????
    UCSR1A |= (1 << U2X);
    UCSR1B = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
    UCSR1C = (1 << UCSZ1) | (1 << UCSZ0);
}

void UART_SendChar(char c) {
    while (!(UCSR1A & (1 << UDRE)));
    UDR1 = c;  
}

void UART_SendString(const char* str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

// ??????????????????';'???
bool UART_GetCommand(char* out_cmd) {
    uint8_t temp_tail = rx_tail;
    bool found = false;
    
    // 1. ?????????????? ';'
    while (temp_tail != rx_head) {
        if (rx_ring[temp_tail] == ';') {
            found = true;
            break;
        }
        temp_tail = (temp_tail + 1) % RX_BUF_SIZE;
    }

    if (!found) return false;

    // 2. ???????????????????
    uint8_t i = 0;
    while (rx_tail != rx_head) {
        char c = rx_ring[rx_tail];
        rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
        
        if (c != '\r' && c != '\n') { 
            out_cmd[i++] = c;
        }
        
        if (c == ';') {
            out_cmd[i] = '\0'; 
            return true;       
        }
    }
    return false;
}

// ??????
ISR(USART1_RX_vect) {
    char c = UDR1;
    uint8_t next_head = (rx_head + 1) % RX_BUF_SIZE;
    if (next_head != rx_tail) { 
        rx_ring[rx_head] = c;
        rx_head = next_head;
    }
}