#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

void UART_Init(void);
void UART_SendChar(char c);
void UART_SendString(const char* str);
bool UART_GetCommand(char* out_cmd); // ???????????

#endif