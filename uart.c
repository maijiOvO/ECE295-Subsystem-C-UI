#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

volatile char rx_buffer[32];
volatile uint8_t rx_index = 0;
volatile bool cmd_ready = false;

void UART_Init(void) {
    // 1MHz ????9600 ???? UBRR ?? 12
    UBRR1H = 0;
    UBRR1L = 12;

    // ?????? (????? 0)
    UCSR1A |= (1 << U2X);

    // ????(RXEN)?????(TXEN)?????????(RXCIE)
    UCSR1B = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);

    // ?????: 8??????????1????
    UCSR1C = (1 << UCSZ1) | (1 << UCSZ0);
}

void UART_SendChar(char c) {
    // ????????? (????? 0)
    while (!(UCSR1A & (1 << UDRE)));
    UDR1 = c;
}

void UART_SendString(const char* str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

void UART_ResetBuffer(void) {
    rx_index = 0;
    cmd_ready = false;
}

// 串口接收中断服务程序 (收到一个字符就会自动触发)
ISR(USART1_RX_vect) {
    char c = UDR1;
    
    // 如果主循环还没处理完上一条命令，丢弃新数据
    if (cmd_ready) return; 

    // CAT协议规定所有命令以分号 ';' 结尾
    if (c == ';') {
        rx_buffer[rx_index] = '\0'; // 加上字符串结尾符
        cmd_ready = true;           // 立起Flag，通知主循环
    } 
    // 忽略回车和换行符，防止干扰
    else if (c != '\r' && c != '\n') { 
        if (rx_index < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_index++] = c;
        }
    }
}