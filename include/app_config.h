#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "stm32f1xx_hal.h"

/* Four LDR voltage dividers are connected to ADC1 channels 0-3. */
#define LDR_UP_CHANNEL              ADC_CHANNEL_0
#define LDR_DOWN_CHANNEL            ADC_CHANNEL_1
#define LDR_LEFT_CHANNEL            ADC_CHANNEL_2
#define LDR_RIGHT_CHANNEL           ADC_CHANNEL_3
#define LDR_AVERAGE_SAMPLES         3U
#define LDR_DIFF_THRESHOLD_CV       20U

/*
 * The thesis code treated a larger ADC-derived value as brighter.
 * Set this to 0 if the physical divider produces a lower voltage in brighter light.
 */
#define LDR_HIGHER_READING_IS_BRIGHTER  1U

/* TIM1 PWM outputs: CH1=PA8 and CH2=PA9 on STM32F103C8T6. */
#define SERVO_HORIZONTAL_CHANNEL    TIM_CHANNEL_1
#define SERVO_VERTICAL_CHANNEL      TIM_CHANNEL_2
#define SERVO_MIN_US                1750U
#define SERVO_CENTER_US             1850U
#define SERVO_MAX_US                1950U
#define SERVO_STEP_US               10U

/* Buttons are active-low and use the STM32 internal pull-up resistors. */
#define BUTTON_PORT                 GPIOB
#define BUTTON_MODE_PIN             GPIO_PIN_10
#define BUTTON_UP_PIN               GPIO_PIN_12
#define BUTTON_RIGHT_PIN            GPIO_PIN_13
#define BUTTON_LEFT_PIN             GPIO_PIN_14
#define BUTTON_DOWN_PIN             GPIO_PIN_15
#define BUTTON_DEBOUNCE_MS          25U

/* SSD1306 128x64 OLED on I2C1: PB6=SCL, PB7=SDA. */
#define OLED_I2C_ADDRESS            (0x3CU << 1)
#define OLED_WIDTH                  128U
#define OLED_HEIGHT                 64U

/* DS18B20 one-wire data pin. An external 4.7 kOhm pull-up is required. */
#define DS18B20_PORT                GPIOB
#define DS18B20_PIN                 GPIO_PIN_1
#define DS18B20_CONVERSION_MS       750U

/* Alarm outputs. Change active levels if the modules are active-low. */
#define ALARM_PORT                  GPIOB
#define ALARM_BUZZER_PIN            GPIO_PIN_8
#define ALARM_LED_PIN               GPIO_PIN_9
#define ALARM_ACTIVE_STATE          GPIO_PIN_SET
#define ALARM_INACTIVE_STATE        GPIO_PIN_RESET
#define TEMPERATURE_ALARM_C         60.0f

/* Cooperative task periods used by the application loop. */
#define CONTROL_PERIOD_MS           50U
#define DISPLAY_PERIOD_MS           200U

#endif
