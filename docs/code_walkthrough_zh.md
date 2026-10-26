# STM32双轴太阳能追光系统代码讲解

这份说明按照代码实际执行顺序讲解。空行、花括号和纯数据字体表不逐项重复；其余声明、判断、函数调用和配置项都说明其作用。

## 1 项目如何运行

上电后的执行顺序如下：

1. HAL初始化并建立72 MHz系统时钟。
2. 初始化DWT微秒计时器，供DS18B20一线协议使用。
3. 初始化按键、声光报警、温度传感器和追光状态机。
4. 初始化ADC、TIM1 PWM和OLED；任意关键外设失败则进入错误处理。
5. 主循环持续扫描按键。
6. 每50 ms读取四路光敏电阻并执行一次自动追光。
7. DS18B20以非阻塞方式启动转换，等待750 ms后读取结果。
8. 每200 ms刷新一次OLED。

## 2 platformio.ini

```ini
[env:bluepill_f103c8]
```

建立名为`bluepill_f103c8`的PlatformIO编译环境。

```ini
platform = ststm32
board = bluepill_f103c8
framework = stm32cube
```

- `ststm32`选择STM32平台支持包。
- `bluepill_f103c8`选择STM32F103C8T6 Blue Pill开发板定义。
- `stm32cube`表示项目使用STM32 HAL库。

```ini
upload_protocol = stlink
monitor_speed = 115200
```

- 使用ST-Link烧录程序。
- 预留115200波特率的串口监视配置；当前程序没有输出串口日志。

```ini
build_flags =
    -D HSE_VALUE=8000000U
    -Wall
    -Wextra
```

- 告诉HAL外部晶振为8 MHz。
- 打开常见和额外编译警告，帮助发现类型转换、未使用变量等问题。

## 3 app_config.h

这个文件把所有可能因实物接线而变化的内容集中管理。修改引脚、阈值或舵机行程时，不必进入驱动文件。

```c
#include "stm32f1xx_hal.h"
```

引入STM32F1 HAL的GPIO、ADC、TIM和I2C类型及宏定义。

```c
#define LDR_UP_CHANNEL ADC_CHANNEL_0
...
#define LDR_RIGHT_CHANNEL ADC_CHANNEL_3
```

把上、下、左、右四个光敏分压信号映射到ADC1通道0-3，也就是PA0-PA3。

```c
#define LDR_AVERAGE_SAMPLES 3U
#define LDR_DIFF_THRESHOLD_CV 20U
```

- 每一路ADC连续读取三次并取均值，减小随机波动。
- 只有两个方向相差超过20 centivolts，即0.20 V时才移动舵机，形成死区，避免来回抖动。

```c
#define LDR_HIGHER_READING_IS_BRIGHTER 1U
```

保留论文代码“数值越大代表越亮”的判断。如果实际分压电路在强光下电压下降，应改为`0U`。

```c
#define SERVO_MIN_US 1750U
#define SERVO_CENTER_US 1850U
#define SERVO_MAX_US 1950U
#define SERVO_STEP_US 10U
```

- 舵机脉宽被限制在1750-1950 us，防止机构碰撞。
- 上电初始脉宽为1850 us。
- 每次追光或按键操作改变10 us。

按键、OLED、DS18B20、报警部分的宏分别定义端口、引脚、I2C地址、转换等待时间、报警温度和任务周期。所有数字后面的`U`表示无符号整数，避免编译器进行不必要的有符号转换。

## 4 main.c

### 4.1 头文件

```c
#include "alarm.h"
...
#include "tracker.h"
```

每个功能只通过自己的头文件暴露接口。`main.c`不直接操作ADC寄存器、PWM寄存器或I2C数据，降低模块耦合。

```c
#include "stm32f1xx_hal.h"
#include <stdio.h>
```

- HAL头文件提供系统初始化和毫秒计时函数。
- `stdio.h`提供`snprintf`，用于构造OLED显示字符串。

### 4.2 main函数

```c
HAL_Init();
```

复位HAL内部状态，配置SysTick，使`HAL_GetTick()`和`HAL_Delay()`可用。

```c
SystemClock_Config();
```

把8 MHz外部晶振通过PLL倍频到72 MHz。

```c
DelayUs_Init();
```

打开Cortex-M3的DWT周期计数器，之后可以产生DS18B20需要的微秒级时序。

```c
Buttons_Init();
Alarm_Init();
DS18B20_Init();
Tracker_Init();
```

依次配置按键输入、LED/蜂鸣器输出、一线温度传感器引脚和默认自动模式。这些初始化不依赖ADC、PWM或OLED是否成功。

```c
if (!LDR_Init() || !Servo_Init() || !OLED_Init()) {
    Error_Handler();
}
```

利用C语言短路求值依次初始化ADC、PWM和OLED。任意函数返回`false`，程序进入错误处理，避免在外设未就绪时继续运行。

```c
LdrReadings readings = {0};
uint32_t last_control_ms = 0U;
uint32_t last_display_ms = 0U;
```

- `readings`保存四路最新光照电压。
-两个时间戳用于控制50 ms和200 ms周期任务。

```c
while (1) {
```

MCU进入永久主循环。

```c
const uint32_t now_ms = HAL_GetTick();
```

只在本轮开始读取一次系统毫秒时间，保证本轮所有时间判断基于同一时刻。

```c
const ButtonEvent button_event = Buttons_Poll(now_ms);
if (button_event != BUTTON_EVENT_NONE) {
    Tracker_HandleButton(button_event);
}
```

扫描并消抖五个按键。如果检测到一次新的按下事件，交给追光状态机处理。按住不放不会每一轮重复触发。

```c
if ((uint32_t)(now_ms - last_control_ms) >= CONTROL_PERIOD_MS) {
```

用无符号减法判断控制周期是否到达；即使32位毫秒计数器溢出，这种写法仍能正常工作。

```c
last_control_ms = now_ms;
if (LDR_ReadAll(&readings)) {
    Tracker_Update(&readings);
}
```

更新时间戳，读取四路光敏电压。只有全部ADC读取成功才执行追光，避免用不完整数据驱动舵机。

```c
if (DS18B20_Task(now_ms) && DS18B20_HasValidTemperature()) {
    Alarm_Set(DS18B20_GetTemperatureC() > TEMPERATURE_ALARM_C);
}
```

温度任务第一次调用只启动转换；750 ms后再次调用才读取温度。读取到新的有效温度后，根据阈值开关声光报警。

```c
if ((uint32_t)(now_ms - last_display_ms) >= DISPLAY_PERIOD_MS) {
    last_display_ms = now_ms;
    Display_Status(&readings);
}
```

每200 ms刷新显示。OLED不必像控制器那样高频更新，降低I2C通信负担。

### 4.3 Display_Status

```c
char line[22];
OLED_Clear();
```

建立一行文本缓冲区并清空内存帧缓存。128像素宽、每字符6像素时，一行最多约21个字符，因此数组留22字节容纳结尾`\0`。

每一组`OLED_SetCursor`、`snprintf`和`OLED_WriteString`完成一行显示：

- 第0页：上、下光照电压；
- 第1页：左、右光照电压；
- 第2页：自动/手动模式和水平舵机脉宽；
- 第3页：垂直舵机脉宽；
- 第4页：温度或等待提示；
- 第5页：报警状态。

```c
const int16_t temperature_tenths = (int16_t)(temperature * 10.0f);
```

把浮点温度转换成十分之一摄氏度的整数。这样无需启用体积较大的浮点`printf`支持。

```c
(void)OLED_Update();
```

把1024字节帧缓存分8页发送到SSD1306。返回值暂时忽略，所以显式转换成`void`以消除警告。

### 4.4 SystemClock_Config

`RCC_OscInitTypeDef`描述振荡器和PLL，`RCC_ClkInitTypeDef`描述系统总线分频。

- HSE保持8 MHz；
- PLL乘9得到72 MHz SYSCLK；
- AHB保持72 MHz；
- APB1除2得到36 MHz，满足F1限制；
- APB2保持72 MHz，供TIM1和ADC相关总线使用；
- Flash等待周期设置为2，适配72 MHz。

任意时钟配置失败都会进入`Error_Handler()`。

## 5 ldr.c

```c
static ADC_HandleTypeDef hadc1;
```

ADC句柄只在本文件可见，其他模块不能绕过LDR接口直接修改ADC配置。

`LDR_ReadChannel()`执行以下步骤：

1. 把目标ADC通道设为规则组第1位；
2. 选择55.5周期采样时间，使高阻值LDR分压信号有更充分的采样时间；
3. 循环三次启动转换、等待完成并累加结果；
4. 计算12位ADC平均值；
5. 用`average_raw * 330 / 4095`换算成centivolts，满量程对应3.30 V；
6. 任一步HAL操作失败则返回`false`。

`LDR_Init()`打开GPIOA和ADC1时钟，把PA0-PA3配置为模拟输入，设置单次软件触发转换，并执行ADC校准。

`LDR_ReadAll()`先检查输出指针，再使用逻辑与依次读取四路。逻辑与具有短路性质：前一路失败后不再继续，函数整体返回`false`。

## 6 servo.c

```c
static TIM_HandleTypeDef htim1;
static uint16_t horizontal_pulse_us = SERVO_CENTER_US;
static uint16_t vertical_pulse_us = SERVO_CENTER_US;
```

保存TIM1句柄和两个轴当前脉宽。

`Servo_ClampPulse()`把任意计算结果限制在安全上下限内。使用`int32_t`输入是为了正确处理负方向调整后可能出现的小于零结果。

```c
htim1.Init.Prescaler = 72U - 1U;
htim1.Init.Period = 20000U - 1U;
```

72 MHz计数时钟除以72得到1 MHz，即每个计数1 us；计满20000次产生20 ms周期，也就是舵机使用的50 Hz PWM。

```c
pwm_config.OCMode = TIM_OCMODE_PWM1;
pwm_config.Pulse = SERVO_CENTER_US;
```

选择PWM模式1，并以中心脉宽初始化两个通道。

`Servo_SetPulse()`先限幅，再更新软件变量和TIM比较寄存器。`Servo_Adjust()`在当前脉宽基础上增加或减少步长。`Servo_GetPulse()`为OLED和控制模块提供只读状态。

## 7 buttons.c

`ButtonState`为每个按键保存：引脚、对应事件、上一次原始电平、已经确认的稳定状态以及电平变化时间。

`buttons[]`建立五个按键的静态映射表。

`Buttons_Init()`把五个引脚配置为上拉输入，因此松开时为高，按下接地时为低。

`Buttons_Poll()`逐个读取按键：

1. 如果原始电平变化，记录变化时刻；
2. 只有该状态持续至少25 ms才被认为稳定；
3. 只有从松开变为稳定按下时返回一次事件；
4. 没有事件则返回`BUTTON_EVENT_NONE`。

## 8 tracker.c

```c
static TrackerMode tracker_mode = TRACKER_MODE_AUTOMATIC;
```

状态机上电默认自动追光。

`Tracker_FirstDirectionIsBrighter()`根据配置决定“大电压更亮”还是“小电压更亮”，同时把0.20 V死区加入比较。

`Tracker_HandleButton()`的行为与论文主程序保持一致：

- MODE：自动和手动互相切换；
- UP/DOWN：切入手动模式并改变垂直舵机；
- LEFT/RIGHT：切入手动模式并改变水平舵机；
- NONE：不执行动作。

`Tracker_Update()`只在读数指针有效且当前为自动模式时工作。上/下和左/右分别比较，因此一次控制周期可以同时调整两个轴。差值落入死区时不动作。

## 9 ds18b20.c

DS18B20通过一根数据线通信，所有字节均最低有效位先发送。

- `OneWire_DriveLow()`主动把总线拉低；
- `OneWire_Release()`在开漏模式下释放总线，由4.7 kOhm电阻拉高；
- `OneWire_Reset()`拉低480 us并检测传感器返回的presence脉冲；
- `OneWire_WriteBit()`根据0或1产生不同低电平宽度；
- `OneWire_ReadBit()`发起读时隙并在规定时间采样；
- `OneWire_WriteByte()`和`OneWire_ReadByte()`循环8次完成一个字节；
- `DS18B20_Crc8()`使用Dallas/Maxim多项式检查9字节scratchpad；
- `DS18B20_StartConversion()`发送Skip ROM和Convert T；
- `DS18B20_ReadTemperature()`读取scratchpad，把16位有符号原始值除以16得到摄氏度。

`DS18B20_Task()`使用状态变量避免阻塞750 ms：未启动时开始转换；转换期间立即返回；时间到后读取结果并保存。主循环因此可以继续扫描按键、控制舵机和更新OLED。

## 10 ssd1306.c和font5x7.c

```c
static uint8_t framebuffer[OLED_WIDTH * OLED_HEIGHT / 8U];
```

SSD1306每个字节表示垂直方向8个像素，128x64屏幕需要1024字节缓存。

`OLED_WriteCommand()`在命令前发送控制字节`0x00`。数据传输时使用控制字节`0x40`。

`OLED_Init()`完成PB6/PB7开漏复用、400 kHz I2C配置和SSD1306初始化命令发送。

`OLED_SetCursor()`使用字符列和8像素页面定位。`OLED_WriteChar()`从字体表取出5列像素，再增加1列空白，因此一个字符宽6像素。`OLED_WriteString()`持续写字符直到遇到字符串结束符。

`OLED_Update()`逐页设置SSD1306页地址和列地址，然后发送128字节显示数据。

`font5x7.c`中的每个5字节数组代表一个字符的5列像素。位值为1的像素点亮。数字和大写字母按数组索引查找，冒号、小数点和负号通过`switch`选择；未支持字符显示问号形状。

## 11 alarm.c

`Alarm_Init()`把PB8和PB9配置为推挽输出并默认关闭。`Alarm_Set()`同时更新软件状态、LED和蜂鸣器输出。`Alarm_IsActive()`让显示模块读取当前报警状态。

## 12 delay_us.c

```c
CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
DWT->CYCCNT = 0U;
DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
```

三行依次打开调试跟踪模块、清零周期计数器并启动计数。

`DelayUs()`把微秒数乘以每微秒CPU周期数，记录开始计数，然后等待经过指定周期。无符号减法使DWT计数溢出时仍然有效。这个忙等待只用于几十到几百微秒的一线总线时序，不能用于长时间任务。

## 13 第一次上电建议

1. 暂时拆下两个舵机，只确认OLED和四路ADC数值变化。
2. 遮挡某一个LDR，检查对应方向显示是否正确。
3. 确认强光下电压是增大还是减小，必要时修改`LDR_HIGHER_READING_IS_BRIGHTER`。
4. 单独连接一个舵机，先把上下限设置得更窄，确认运动方向。
5. 接入第二个舵机并检查机械结构是否会卡死。
6. 接入DS18B20，确认温度稳定后再连接蜂鸣器。
7. 最后开启自动模式，并逐步调整死区和步长。
