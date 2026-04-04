#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

// 供主循环读取的全局变量
extern volatile char rx_buffer[32];
extern volatile bool cmd_ready;

void UART_Init(void);
void UART_SendChar(char c);
void UART_SendString(const char* str);
void UART_ResetBuffer(void);

#endif