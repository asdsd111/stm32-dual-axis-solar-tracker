#ifndef ALARM_H
#define ALARM_H

#include <stdbool.h>

void Alarm_Init(void);
void Alarm_Set(bool enabled);
bool Alarm_IsActive(void);

#endif
