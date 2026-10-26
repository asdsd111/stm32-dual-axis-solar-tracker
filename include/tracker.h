#ifndef TRACKER_H
#define TRACKER_H

#include "buttons.h"
#include "ldr.h"
#include <stdbool.h>

typedef enum {
    TRACKER_MODE_AUTOMATIC = 0,
    TRACKER_MODE_MANUAL
} TrackerMode;

void Tracker_Init(void);
void Tracker_HandleButton(ButtonEvent event);
void Tracker_Update(const LdrReadings *readings);
TrackerMode Tracker_GetMode(void);

#endif
