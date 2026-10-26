#include "alarm.h"
#include "app_config.h"
#include "buttons.h"
#include "delay_us.h"
#include "ds18b20.h"
#include "ldr.h"
#include "servo.h"
#include "ssd1306.h"
#include "tracker.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>

static void SystemClock_Config(void);
static void Display_Status(const LdrReadings *readings);
static void Error_Handler(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    DelayUs_Init();

    Buttons_Init();
    Alarm_Init();
    DS18B20_Init();
    Tracker_Init();

    if (!LDR_Init() || !Servo_Init() || !OLED_Init()) {
        Error_Handler();
    }

    LdrReadings readings = {0};
    uint32_t last_control_ms = 0U;
    uint32_t last_display_ms = 0U;

    while (1) {
        const uint32_t now_ms = HAL_GetTick();
        const ButtonEvent button_event = Buttons_Poll(now_ms);

        if (button_event != BUTTON_EVENT_NONE) {
            Tracker_HandleButton(button_event);
        }

        if ((uint32_t)(now_ms - last_control_ms) >= CONTROL_PERIOD_MS) {
            last_control_ms = now_ms;
            if (LDR_ReadAll(&readings)) {
                Tracker_Update(&readings);
            }
        }

        if (DS18B20_Task(now_ms) && DS18B20_HasValidTemperature()) {
            Alarm_Set(DS18B20_GetTemperatureC() > TEMPERATURE_ALARM_C);
        }

        if ((uint32_t)(now_ms - last_display_ms) >= DISPLAY_PERIOD_MS) {
            last_display_ms = now_ms;
            Display_Status(&readings);
        }
    }
}

static void Display_Status(const LdrReadings *readings)
{
    char line[22];

    OLED_Clear();

    OLED_SetCursor(0U, 0U);
    (void)snprintf(line, sizeof(line), "U:%3u D:%3u",
                   (unsigned int)readings->up_cv, (unsigned int)readings->down_cv);
    OLED_WriteString(line);

    OLED_SetCursor(0U, 1U);
    (void)snprintf(line, sizeof(line), "L:%3u R:%3u",
                   (unsigned int)readings->left_cv, (unsigned int)readings->right_cv);
    OLED_WriteString(line);

    OLED_SetCursor(0U, 2U);
    (void)snprintf(line, sizeof(line), "%s H:%4u",
                   Tracker_GetMode() == TRACKER_MODE_AUTOMATIC ? "AUTO" : "MANUAL",
                   (unsigned int)Servo_GetPulse(SERVO_AXIS_HORIZONTAL));
    OLED_WriteString(line);

    OLED_SetCursor(0U, 3U);
    (void)snprintf(line, sizeof(line), "V:%4u",
                   (unsigned int)Servo_GetPulse(SERVO_AXIS_VERTICAL));
    OLED_WriteString(line);

    OLED_SetCursor(0U, 4U);
    if (DS18B20_HasValidTemperature()) {
        const int16_t temperature_tenths = (int16_t)(DS18B20_GetTemperatureC() * 10.0f);
        const int16_t absolute_tenths = (temperature_tenths < 0)
                                      ? (int16_t)-temperature_tenths
                                      : temperature_tenths;
        (void)snprintf(line, sizeof(line), "T:%s%d.%dC",
                       temperature_tenths < 0 ? "-" : "",
                       absolute_tenths / 10, absolute_tenths % 10);
    } else {
        (void)snprintf(line, sizeof(line), "T:WAIT");
    }
    OLED_WriteString(line);

    OLED_SetCursor(0U, 5U);
    OLED_WriteString(Alarm_IsActive() ? "ALARM:ON" : "ALARM:OFF");

    (void)OLED_Update();
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clocks = {0};

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
        Error_Handler();
    }

    clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV2;
    clocks.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* Connect a debugger here to inspect the failed peripheral init. */
    }
}
