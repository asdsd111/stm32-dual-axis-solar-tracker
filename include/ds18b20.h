#ifndef DS18B20_H
#define DS18B20_H

#include <stdbool.h>
#include <stdint.h>

void DS18B20_Init(void);
bool DS18B20_Task(uint32_t now_ms);
bool DS18B20_HasValidTemperature(void);
float DS18B20_GetTemperatureC(void);

#endif
