#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SERVO_AXIS_HORIZONTAL = 0,
    SERVO_AXIS_VERTICAL
} ServoAxis;

bool Servo_Init(void);
void Servo_SetPulse(ServoAxis axis, uint16_t pulse_us);
void Servo_Adjust(ServoAxis axis, int16_t delta_us);
uint16_t Servo_GetPulse(ServoAxis axis);

#endif
