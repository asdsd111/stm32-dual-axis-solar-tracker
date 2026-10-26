#include "servo.h"
#include "app_config.h"
#include "stm32f1xx_hal.h"

static TIM_HandleTypeDef htim1;
static uint16_t horizontal_pulse_us = SERVO_CENTER_US;
static uint16_t vertical_pulse_us = SERVO_CENTER_US;

static uint16_t Servo_ClampPulse(int32_t pulse_us)
{
    if (pulse_us < (int32_t)SERVO_MIN_US) {
        return SERVO_MIN_US;
    }
    if (pulse_us > (int32_t)SERVO_MAX_US) {
        return SERVO_MAX_US;
    }
    return (uint16_t)pulse_us;
}

bool Servo_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_OC_InitTypeDef pwm_config = {0};

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 72U - 1U;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 20000U - 1U;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0U;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        return false;
    }

    pwm_config.OCMode = TIM_OCMODE_PWM1;
    pwm_config.Pulse = SERVO_CENTER_US;
    pwm_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    pwm_config.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    pwm_config.OCFastMode = TIM_OCFAST_DISABLE;
    pwm_config.OCIdleState = TIM_OCIDLESTATE_RESET;
    pwm_config.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &pwm_config, SERVO_HORIZONTAL_CHANNEL) != HAL_OK) {
        return false;
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &pwm_config, SERVO_VERTICAL_CHANNEL) != HAL_OK) {
        return false;
    }
    if (HAL_TIM_PWM_Start(&htim1, SERVO_HORIZONTAL_CHANNEL) != HAL_OK) {
        return false;
    }
    if (HAL_TIM_PWM_Start(&htim1, SERVO_VERTICAL_CHANNEL) != HAL_OK) {
        return false;
    }

    Servo_SetPulse(SERVO_AXIS_HORIZONTAL, SERVO_CENTER_US);
    Servo_SetPulse(SERVO_AXIS_VERTICAL, SERVO_CENTER_US);
    return true;
}

void Servo_SetPulse(ServoAxis axis, uint16_t pulse_us)
{
    const uint16_t safe_pulse = Servo_ClampPulse((int32_t)pulse_us);

    if (axis == SERVO_AXIS_HORIZONTAL) {
        horizontal_pulse_us = safe_pulse;
        __HAL_TIM_SET_COMPARE(&htim1, SERVO_HORIZONTAL_CHANNEL, safe_pulse);
    } else {
        vertical_pulse_us = safe_pulse;
        __HAL_TIM_SET_COMPARE(&htim1, SERVO_VERTICAL_CHANNEL, safe_pulse);
    }
}

void Servo_Adjust(ServoAxis axis, int16_t delta_us)
{
    const int32_t current_pulse = (int32_t)Servo_GetPulse(axis);
    Servo_SetPulse(axis, Servo_ClampPulse(current_pulse + delta_us));
}

uint16_t Servo_GetPulse(ServoAxis axis)
{
    return (axis == SERVO_AXIS_HORIZONTAL) ? horizontal_pulse_us : vertical_pulse_us;
}
