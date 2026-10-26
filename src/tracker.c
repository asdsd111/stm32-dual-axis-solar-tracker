#include "tracker.h"
#include "app_config.h"
#include "servo.h"
#include <stdint.h>
#include <stddef.h>

static TrackerMode tracker_mode = TRACKER_MODE_AUTOMATIC;

static bool Tracker_FirstDirectionIsBrighter(uint16_t first, uint16_t second)
{
#if LDR_HIGHER_READING_IS_BRIGHTER
    return first > (uint16_t)(second + LDR_DIFF_THRESHOLD_CV);
#else
    return second > (uint16_t)(first + LDR_DIFF_THRESHOLD_CV);
#endif
}

void Tracker_Init(void)
{
    tracker_mode = TRACKER_MODE_AUTOMATIC;
}

void Tracker_HandleButton(ButtonEvent event)
{
    switch (event) {
    case BUTTON_EVENT_MODE:
        tracker_mode = (tracker_mode == TRACKER_MODE_AUTOMATIC)
                     ? TRACKER_MODE_MANUAL
                     : TRACKER_MODE_AUTOMATIC;
        break;
    case BUTTON_EVENT_UP:
        tracker_mode = TRACKER_MODE_MANUAL;
        Servo_Adjust(SERVO_AXIS_VERTICAL, (int16_t)SERVO_STEP_US);
        break;
    case BUTTON_EVENT_DOWN:
        tracker_mode = TRACKER_MODE_MANUAL;
        Servo_Adjust(SERVO_AXIS_VERTICAL, -(int16_t)SERVO_STEP_US);
        break;
    case BUTTON_EVENT_LEFT:
        tracker_mode = TRACKER_MODE_MANUAL;
        Servo_Adjust(SERVO_AXIS_HORIZONTAL, (int16_t)SERVO_STEP_US);
        break;
    case BUTTON_EVENT_RIGHT:
        tracker_mode = TRACKER_MODE_MANUAL;
        Servo_Adjust(SERVO_AXIS_HORIZONTAL, -(int16_t)SERVO_STEP_US);
        break;
    case BUTTON_EVENT_NONE:
    default:
        break;
    }
}

void Tracker_Update(const LdrReadings *readings)
{
    if (readings == NULL || tracker_mode != TRACKER_MODE_AUTOMATIC) {
        return;
    }

    if (Tracker_FirstDirectionIsBrighter(readings->up_cv, readings->down_cv)) {
        Servo_Adjust(SERVO_AXIS_VERTICAL, (int16_t)SERVO_STEP_US);
    } else if (Tracker_FirstDirectionIsBrighter(readings->down_cv, readings->up_cv)) {
        Servo_Adjust(SERVO_AXIS_VERTICAL, -(int16_t)SERVO_STEP_US);
    }

    if (Tracker_FirstDirectionIsBrighter(readings->left_cv, readings->right_cv)) {
        Servo_Adjust(SERVO_AXIS_HORIZONTAL, (int16_t)SERVO_STEP_US);
    } else if (Tracker_FirstDirectionIsBrighter(readings->right_cv, readings->left_cv)) {
        Servo_Adjust(SERVO_AXIS_HORIZONTAL, -(int16_t)SERVO_STEP_US);
    }
}

TrackerMode Tracker_GetMode(void)
{
    return tracker_mode;
}
