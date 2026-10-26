#ifndef LDR_H
#define LDR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t up_cv;
    uint16_t down_cv;
    uint16_t left_cv;
    uint16_t right_cv;
} LdrReadings;

bool LDR_Init(void);
bool LDR_ReadAll(LdrReadings *readings);

#endif
