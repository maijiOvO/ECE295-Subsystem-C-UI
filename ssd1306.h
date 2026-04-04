#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SCREEN_L_ADDR 0x3C  // left screen addr
#define SCREEN_R_ADDR 0x3D  // right screen addr

#define OLED_COLOR_BLACK 0x00
#define OLED_COLOR_WHITE 0xFF

void OLED_Init(uint8_t addr);
void OLED_Fill(uint8_t addr, uint8_t data);
void OLED_Clear(uint8_t addr);
void OLED_SetPos(uint8_t addr, uint8_t x, uint8_t y);

#define OLED_MODE_NORMAL 0
#define OLED_MODE_INVERT 1

void OLED_ShowChar(uint8_t addr, uint8_t x, uint8_t y, char chr, uint8_t mode);
void OLED_ShowString(uint8_t addr, uint8_t x, uint8_t y, char *str, uint8_t mode);

#endif