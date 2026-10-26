#include "delay_us.h"
#include "stm32f1xx_hal.h"

void DelayUs_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DelayUs(uint32_t microseconds)
{
    const uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    const uint32_t wait_cycles = microseconds * cycles_per_us;
    const uint32_t start_cycles = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start_cycles) < wait_cycles) {
        /* Busy-wait only for short one-wire timing intervals. */
    }
}
