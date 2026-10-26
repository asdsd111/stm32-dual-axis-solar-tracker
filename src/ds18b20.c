#include "ds18b20.h"
#include "app_config.h"
#include "delay_us.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

#define DS18B20_CMD_SKIP_ROM        0xCCU
#define DS18B20_CMD_CONVERT_T       0x44U
#define DS18B20_CMD_READ_SCRATCHPAD 0xBEU

static bool conversion_running = false;
static bool temperature_valid = false;
static uint32_t conversion_started_ms = 0U;
static float last_temperature_c = 0.0f;

static void OneWire_DriveLow(void)
{
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
}

static void OneWire_Release(void)
{
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
}

static bool OneWire_Reset(void)
{
    OneWire_DriveLow();
    DelayUs(480U);
    OneWire_Release();
    DelayUs(70U);
    const bool presence_detected = HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_RESET;
    DelayUs(410U);
    return presence_detected;
}

static void OneWire_WriteBit(uint8_t bit_value)
{
    OneWire_DriveLow();
    if (bit_value != 0U) {
        DelayUs(6U);
        OneWire_Release();
        DelayUs(64U);
    } else {
        DelayUs(60U);
        OneWire_Release();
        DelayUs(10U);
    }
}

static uint8_t OneWire_ReadBit(void)
{
    OneWire_DriveLow();
    DelayUs(3U);
    OneWire_Release();
    DelayUs(10U);
    const uint8_t bit_value = (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_SET) ? 1U : 0U;
    DelayUs(53U);
    return bit_value;
}

static void OneWire_WriteByte(uint8_t byte_value)
{
    for (uint8_t bit_index = 0U; bit_index < 8U; ++bit_index) {
        OneWire_WriteBit((uint8_t)((byte_value >> bit_index) & 0x01U));
    }
}

static uint8_t OneWire_ReadByte(void)
{
    uint8_t byte_value = 0U;

    for (uint8_t bit_index = 0U; bit_index < 8U; ++bit_index) {
        byte_value |= (uint8_t)(OneWire_ReadBit() << bit_index);
    }

    return byte_value;
}

static uint8_t DS18B20_Crc8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0U;

    for (uint8_t byte_index = 0U; byte_index < length; ++byte_index) {
        uint8_t current_byte = data[byte_index];
        for (uint8_t bit_index = 0U; bit_index < 8U; ++bit_index) {
            const uint8_t mix = (uint8_t)((crc ^ current_byte) & 0x01U);
            crc >>= 1U;
            if (mix != 0U) {
                crc ^= 0x8CU;
            }
            current_byte >>= 1U;
        }
    }

    return crc;
}

static bool DS18B20_StartConversion(void)
{
    if (!OneWire_Reset()) {
        return false;
    }

    OneWire_WriteByte(DS18B20_CMD_SKIP_ROM);
    OneWire_WriteByte(DS18B20_CMD_CONVERT_T);
    return true;
}

static bool DS18B20_ReadTemperature(float *temperature_c)
{
    uint8_t scratchpad[9] = {0};

    if (temperature_c == NULL || !OneWire_Reset()) {
        return false;
    }

    OneWire_WriteByte(DS18B20_CMD_SKIP_ROM);
    OneWire_WriteByte(DS18B20_CMD_READ_SCRATCHPAD);

    for (uint8_t index = 0U; index < 9U; ++index) {
        scratchpad[index] = OneWire_ReadByte();
    }

    if (DS18B20_Crc8(scratchpad, 8U) != scratchpad[8]) {
        return false;
    }

    const int16_t raw_temperature = (int16_t)(((uint16_t)scratchpad[1] << 8U) | scratchpad[0]);
    *temperature_c = (float)raw_temperature / 16.0f;
    return true;
}

void DS18B20_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = DS18B20_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_PORT, &gpio);

    OneWire_Release();
    conversion_running = false;
    temperature_valid = false;
}

bool DS18B20_Task(uint32_t now_ms)
{
    if (!conversion_running) {
        if (DS18B20_StartConversion()) {
            conversion_running = true;
            conversion_started_ms = now_ms;
        }
        return false;
    }

    if ((uint32_t)(now_ms - conversion_started_ms) < DS18B20_CONVERSION_MS) {
        return false;
    }

    conversion_running = false;
    float measured_temperature_c = 0.0f;
    if (!DS18B20_ReadTemperature(&measured_temperature_c)) {
        return false;
    }

    last_temperature_c = measured_temperature_c;
    temperature_valid = true;
    return true;
}

bool DS18B20_HasValidTemperature(void)
{
    return temperature_valid;
}

float DS18B20_GetTemperatureC(void)
{
    return last_temperature_c;
}
