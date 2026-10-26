#include "ldr.h"
#include "app_config.h"
#include "stm32f1xx_hal.h"
#include <stddef.h>

static ADC_HandleTypeDef hadc1;

static bool LDR_ReadChannel(uint32_t channel, uint16_t *centivolts)
{
    ADC_ChannelConfTypeDef channel_config = {0};
    uint32_t sample_sum = 0U;

    channel_config.Channel = channel;
    channel_config.Rank = ADC_REGULAR_RANK_1;
    channel_config.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc1, &channel_config) != HAL_OK) {
        return false;
    }

    for (uint32_t sample = 0U; sample < LDR_AVERAGE_SAMPLES; ++sample) {
        if (HAL_ADC_Start(&hadc1) != HAL_OK) {
            return false;
        }
        if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK) {
            (void)HAL_ADC_Stop(&hadc1);
            return false;
        }

        sample_sum += HAL_ADC_GetValue(&hadc1);
        (void)HAL_ADC_Stop(&hadc1);
    }

    const uint32_t average_raw = sample_sum / LDR_AVERAGE_SAMPLES;
    *centivolts = (uint16_t)((average_raw * 330U) / 4095U);
    return true;
}

bool LDR_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);

    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1U;

    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        return false;
    }

    return HAL_ADCEx_Calibration_Start(&hadc1) == HAL_OK;
}

bool LDR_ReadAll(LdrReadings *readings)
{
    if (readings == NULL) {
        return false;
    }

    return LDR_ReadChannel(LDR_UP_CHANNEL, &readings->up_cv)
        && LDR_ReadChannel(LDR_DOWN_CHANNEL, &readings->down_cv)
        && LDR_ReadChannel(LDR_LEFT_CHANNEL, &readings->left_cv)
        && LDR_ReadChannel(LDR_RIGHT_CHANNEL, &readings->right_cv);
}
