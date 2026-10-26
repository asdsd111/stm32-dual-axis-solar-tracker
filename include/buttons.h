#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_MODE,
    BUTTON_EVENT_UP,
    BUTTON_EVENT_RIGHT,
    BUTTON_EVENT_LEFT,
    BUTTON_EVENT_DOWN
} ButtonEvent;

void Buttons_Init(void);
ButtonEvent Buttons_Poll(uint32_t now_ms);

#endif
