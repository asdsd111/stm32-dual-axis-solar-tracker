#include "alarm.h"
#include "app_config.h"
#include "stm32f1xx_hal.h"

static bool alarm_active = false;

void Alarm_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = ALARM_BUZZER_PIN | ALARM_LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ALARM_PORT, &gpio);

    Alarm_Set(false);
}

void Alarm_Set(bool enabled)
{
    alarm_active = enabled;
    const GPIO_PinState state = enabled ? ALARM_ACTIVE_STATE : ALARM_INACTIVE_STATE;
    HAL_GPIO_WritePin(ALARM_PORT, ALARM_BUZZER_PIN | ALARM_LED_PIN, state);
}

bool Alarm_IsActive(void)
{
    return alarm_active;
}
