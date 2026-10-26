#include "ssd1306.h"
#include "app_config.h"
#include "font5x7.h"
#include "stm32f1xx_hal.h"
#include <string.h>

static I2C_HandleTypeDef hi2c1;
static uint8_t framebuffer[OLED_WIDTH * OLED_HEIGHT / 8U];
static uint8_t cursor_column = 0U;
static uint8_t cursor_page = 0U;

static bool OLED_WriteCommand(uint8_t command)
{
    uint8_t packet[2] = {0x00U, command};
    return HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDRESS, packet, sizeof(packet), 100U) == HAL_OK;
}

bool OLED_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000U;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0U;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0U;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        return false;
    }

    HAL_Delay(100U);
    const uint8_t init_commands[] = {
        0xAEU, 0x20U, 0x02U, 0xB0U, 0xC8U, 0x00U, 0x10U, 0x40U,
        0x81U, 0x7FU, 0xA1U, 0xA6U, 0xA8U, 0x3FU, 0xA4U, 0xD3U,
        0x00U, 0xD5U, 0x80U, 0xD9U, 0xF1U, 0xDAU, 0x12U, 0xDBU,
        0x40U, 0x8DU, 0x14U, 0xAFU
    };

    for (uint32_t index = 0U; index < sizeof(init_commands); ++index) {
        if (!OLED_WriteCommand(init_commands[index])) {
            return false;
        }
    }

    OLED_Clear();
    return OLED_Update();
}

void OLED_Clear(void)
{
    memset(framebuffer, 0, sizeof(framebuffer));
    cursor_column = 0U;
    cursor_page = 0U;
}

void OLED_SetCursor(uint8_t column, uint8_t page)
{
    cursor_column = column;
    cursor_page = page;
}

void OLED_WriteChar(char character)
{
    if (cursor_page >= (OLED_HEIGHT / 8U) || cursor_column >= (OLED_WIDTH / 6U)) {
        return;
    }

    const uint8_t *glyph = Font5x7_GetGlyph(character);
    const uint16_t base_index = (uint16_t)cursor_page * OLED_WIDTH + (uint16_t)cursor_column * 6U;

    for (uint8_t column = 0U; column < 5U; ++column) {
        framebuffer[base_index + column] = glyph[column];
    }
    framebuffer[base_index + 5U] = 0x00U;
    ++cursor_column;
}

void OLED_WriteString(const char *text)
{
    if (text == NULL) {
        return;
    }

    while (*text != '\0') {
        OLED_WriteChar(*text);
        ++text;
    }
}

bool OLED_Update(void)
{
    uint8_t data_packet[OLED_WIDTH + 1U];
    data_packet[0] = 0x40U;

    for (uint8_t page = 0U; page < (OLED_HEIGHT / 8U); ++page) {
        if (!OLED_WriteCommand((uint8_t)(0xB0U + page))
            || !OLED_WriteCommand(0x00U)
            || !OLED_WriteCommand(0x10U)) {
            return false;
        }

        memcpy(&data_packet[1], &framebuffer[(uint16_t)page * OLED_WIDTH], OLED_WIDTH);
        if (HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDRESS,
                                    data_packet, sizeof(data_packet), 100U) != HAL_OK) {
            return false;
        }
    }

    return true;
}
