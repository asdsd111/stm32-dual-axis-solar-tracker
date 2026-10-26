# Reconstruction notes

## What the thesis appendix contained

The available appendix provided one application file containing:

- initialization calls for GPIO, keys, OLED, ADC and two servos;
- four global light-sensor values;
- two servo pulse-width variables;
- a ten-millisecond polling loop;
- automatic/manual mode switching;
- three-sample ADC averaging through `Get_Adc_Average(channel, 3)`;
- differential tracking with a threshold of 20 in the supplied code;
- servo movement in 10 microsecond increments;
- servo pulse limits of 1750-1950 microseconds;
- OLED display calls for four light values and the current mode.

The appendix did not contain the implementation of `gpio`, `key`, `oled`, `adc`, `delay`, `usart`, `motor_duoji` or DS18B20 modules.

## Corrections required for STM32F103C8T6

1. The document referred to a `PB16` button pin. STM32F103 GPIO ports only provide pins 0-15, so the reconstructed design uses PB10 for the mode button.
2. The document associated two TIM1 channels with PA8 and PA11. On the default STM32F103C8T6 mapping, TIM1 CH1 and CH2 are PA8 and PA9. This project therefore uses PA8/PA9.
3. The prose stated that brighter light produces a lower divider voltage, while the original control code treated a larger reading as brighter. `LDR_HIGHER_READING_IS_BRIGHTER` makes this behaviour selectable during hardware calibration.
4. The prose described both 20-unit and 30-unit tracking thresholds. The attached main code used 20, so the reconstruction defaults to 20 centivolts.

## Reconstructed modules

- ADC1 initialization and sequential channel acquisition
- TIM1 50 Hz PWM initialization
- active-low GPIO button input and debounce
- SSD1306 I2C framebuffer driver
- compact 5x7 display font
- DS18B20 one-wire driver with scratchpad CRC verification
- LED and buzzer GPIO outputs
- 72 MHz HSE/PLL clock setup
- cooperative timing for control, display and temperature conversion

## Behaviour intentionally preserved

- automatic mode is selected at startup;
- pressing a direction button enters manual mode;
- the mode button toggles automatic/manual operation;
- vertical movement uses the upper/lower light difference;
- horizontal movement uses the left/right light difference;
- both axes can move during the same control update;
- every movement is clamped to configured pulse limits.

## Hardware-dependent items to verify

- LDR voltage-divider polarity;
- physical orientation of the four LDR sensors;
- servo direction and mechanical travel;
- active level of the buzzer/LED modules;
- SSD1306 I2C address;
- DS18B20 wiring and pull-up resistor;
- whether the Blue Pill board uses a reliable 8 MHz HSE crystal.
