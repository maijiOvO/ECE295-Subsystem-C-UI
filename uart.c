#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"

volatile char rx_buffer[32];
volatile uint8_t rx_index = 0;
volatile bool cmd_ready = false;

void UART_Init(void) {
    // 1MHz 主频下，开启 U2X0 双速模式，9600 波特率的 UBRR 值为 12
    UBRR0H = 0;
    UBRR0L = 12;

    // 开启双速模式 (极大降低 1MHz 主频下的波特率误差)
    UCSR0A |= (1 << U2X0);

    // 允许接收(RXEN0)、允许发送(TXEN0)、开启接收完成中断(RXCIE0)
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

    // 设置帧格式: 8个数据位，无校验位，1个停止位
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void UART_SendChar(char c) {
    // 等待发送缓冲区清空
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
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
ISR(USART0_RX_vect) {
    char c = UDR0;
    
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