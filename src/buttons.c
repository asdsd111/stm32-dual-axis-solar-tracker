#include "buttons.h"
#include "app_config.h"
#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint16_t pin;
    ButtonEvent event;
    bool last_raw_pressed;
    bool stable_pressed;
    uint32_t changed_at_ms;
} ButtonState;

static ButtonState buttons[] = {
    {BUTTON_MODE_PIN, BUTTON_EVENT_MODE, false, false, 0U},
    {BUTTON_UP_PIN, BUTTON_EVENT_UP, false, false, 0U},
    {BUTTON_RIGHT_PIN, BUTTON_EVENT_RIGHT, false, false, 0U},
    {BUTTON_LEFT_PIN, BUTTON_EVENT_LEFT, false, false, 0U},
    {BUTTON_DOWN_PIN, BUTTON_EVENT_DOWN, false, false, 0U}
};

void Buttons_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = BUTTON_MODE_PIN | BUTTON_UP_PIN | BUTTON_RIGHT_PIN
             | BUTTON_LEFT_PIN | BUTTON_DOWN_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BUTTON_PORT, &gpio);
}

ButtonEvent Buttons_Poll(uint32_t now_ms)
{
    const size_t button_count = sizeof(buttons) / sizeof(buttons[0]);

    for (size_t index = 0U; index < button_count; ++index) {
        ButtonState *button = &buttons[index];
        const bool raw_pressed = HAL_GPIO_ReadPin(BUTTON_PORT, button->pin) == GPIO_PIN_RESET;

        if (raw_pressed != button->last_raw_pressed) {
            button->last_raw_pressed = raw_pressed;
            button->changed_at_ms = now_ms;
        }

        if ((uint32_t)(now_ms - button->changed_at_ms) >= BUTTON_DEBOUNCE_MS
            && raw_pressed != button->stable_pressed) {
            button->stable_pressed = raw_pressed;
            if (button->stable_pressed) {
                return button->event;
            }
        }
    }

    return BUTTON_EVENT_NONE;
}
