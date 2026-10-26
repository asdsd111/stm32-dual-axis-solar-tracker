#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include <stdint.h>

bool OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t column, uint8_t page);
void OLED_WriteChar(char character);
void OLED_WriteString(const char *text);
bool OLED_Update(void);

#endif
