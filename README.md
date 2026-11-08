# MAX30102 STM32 健康监测节点

这是一个面向 STM32F103C8T6 的 MAX30102 心率/血氧采集工程，基于原 Keil
工程重构。工程以 100 Hz 红光和红外 PPG 数据为基础，每 5 秒分析一次心率和
SpO2，同时支持 MPU6050 姿态、ADC 模拟量、数字输入、SSD1306 OLED 显示、
本地告警以及可选的 ESP8266 MQTT 上报。

本项目已经使用 Keil MDK-ARM 5.24.2 / ARMCC 5.06 update 5 完整编译通过：

- 默认配置：0 Error，0 Warning。
- Flash：54,020 字节，约 52.8 KiB。
- SRAM：13,352 字节，约 13.0 KiB。
- 启用 MQTT 后：Flash 56,624 字节，约 55.3 KiB；SRAM 14,760 字节，约 14.4 KiB。

> 注意：编译通过不等于硬件已经验证。PPG 结果受佩戴、环境光、LED 电流、
> 运动伪影和算法限制影响，不是医疗诊断设备。

## 主要功能

- MAX30102 启动时校验 PART_ID，确认器件为 0x15。
- 软件 I2C 增加 ACK 检查和忙等超时，不再无期限卡死。
- 使用 INT 引脚等待 PPG 样本，配置为 100 Hz、SpO2 双 LED 模式。
- 维护 500 点滑动窗口，定时执行 Maxim 参考心率/血氧算法。
- 心率、SpO2 分别保留有效标志，并进行生理范围二次校验。
- 心率、血氧、MPU6050 姿态、MPU 温度、ADC 和数字输入集中为健康快照。
- MPU6050 优先使用 DMP 输出姿态；DMP 不可用时用加速度计估算 pitch/roll。
- SSD1306 128x64 OLED 显示 HR、SpO2、压力 ADC/电压、温度、输入和姿态。
- 低 SpO2 可驱动蜂鸣器告警，使用开启/恢复双阈值形成回差。
- ESP8266 可选连接 Wi-Fi 和 MQTT，开机连接一次，运行期异步发布健康 JSON。
- 解析云端 `PowerSwitch` 属性并控制板载状态灯。
- USART1 输出便于串口助手采集的单行健康数据。
- 所有连接、AT 指令、I2C 和 PPG 等待路径都有超时或失败退出。

## 目录结构

```text
MAX30102-Refactored/
├─ USER/
│  ├─ app_config.h            功能开关、引脚、阈值和网络参数
│  ├─ main.c                  初始化与主循环
│  ├─ health_monitor.c/h      采集调度、健康快照、告警和显示
│  ├─ timebase.c/h            TIM2 1 ms 系统时基
│  ├─ IIC.uvprojx             Keil MDK 工程
│  ├─ stm32f10x_conf.h        StdPeriph 头文件裁剪
│  └─ ...                     STM32 启动和中断相关文件
├─ HARDWARE/
│  ├─ IIC/                    MAX30102、软件 I2C、Maxim 算法
│  ├─ MPU6050/                MPU6050 原始数据与 DMP
│  ├─ OLED/                   SSD1306 驱动和 ASCII 字库
│  ├─ ADC/                    ADC1 压力通道
│  ├─ INPUT/                  数字输入
│  ├─ BEE/                    蜂鸣器输出
│  └─ LED/                    板载状态灯
├─ device/
│  ├─ inc/                    ESP8266、USART2、MQTT 接口
│  └─ src/                    AT 指令、MQTT、USART2 接收
├─ SYSTEM/
│  ├─ delay/                  SysTick 短延时
│  ├─ sys/                    中断分组/JTAG 配置
│  └─ usart/                  USART1 printf 输出
├─ STM32F10x_FWLib/           STM32F10x 标准外设库
└─ CORE/                      CMSIS 和启动文件
```

## 硬件建议

必要硬件：

- STM32F103C8T6 最小系统板。
- MAX30102 模块，模块 I2C 侧应带 4.7 kOhm 左右上拉。
- 3.3 V 电源和共地连接。
- USB-TTL，用于观察 USART1 日志。

可选硬件：

- 128x64 SSD1306 OLED，4 线 SPI。
- MPU6050 模块。
- ESP8266/ESP-01S 或兼容的 ESP-AT 模块。
- 有源蜂鸣器。
- 0..3.3 V 模拟传感器。

### 默认引脚映射

| 功能 | 模块引脚 | STM32 引脚 | 说明 |
|---|---|---|---|
| MAX30102 I2C SCL | SCL | PB7 | 外部上拉 |
| MAX30102 I2C SDA | SDA | PB8 | 外部上拉 |
| MAX30102 中断 | INT | PB9 | 低有效，内部上拉 |
| 调试串口 | TXD/RXD | PA9/PA10 | USART1，115200-8-N-1 |
| ESP8266 串口 | TX/RX | PA2/PA3 | USART2 |
| ESP8266 复位 | RST | PA4 | 低电平复位 |
| ESP8266 使能 | CH_PD/EN | PB5 | 高电平使能 |
| 模拟输入 | OUT | PA1 | ADC1_IN1 |
| 数字输入 | OUT | PA5 | 内部上拉 |
| 蜂鸣器 | IN | PA7 | 高电平响 |
| 状态灯 | 板载 LED | PC13 | 低电平点亮 |
| OLED CS | CS | PB12 | 软件片选 |
| OLED RESET | RES | PB13 | 低电平复位 |
| OLED DC | DC | PB14 | 命令/数据选择 |
| OLED SCLK | D0/SCLK | PB15 | 软件 SPI 时钟 |
| OLED SDIN | D1/SDIN | PA8 | 软件 SPI 数据 |
| MPU6050 SCL | SCL | PB10 | 外部上拉 |
| MPU6050 SDA | SDA | PB11 | 外部上拉 |
| MPU6050 AD0 | AD0 | PA15 | 拉低，地址 0x68 |

所有模块必须共地。MAX30102、MPU6050 和 ESP8266 使用 3.3 V，不要把
ESP8266 的 RX 直接接到 5 V 串口输出。ESP8266 发射电流峰值较高，单独提供
稳定 3.3 V、至少 500 mA 的电源更可靠。

### 为什么改动了引脚

原工程的引脚定义存在硬冲突：

- MPU6050 软件 I2C 使用 PB10/PB11，USART3 也使用 PB10/PB11。
- OLED 使用 PA2..PA6，USART2 使用 PA2/PA3，ESP8266 RST 使用 PA4，
  数字输入使用 PA5/PA6，蜂鸣器使用 PA7。
- `SENSOR_Init()` 配置 PA5，但 `SENSOR` 宏读取 PA6。
- 原 `LED_Init()` 注释写 PA8，实际配置 PA13，又把 PA1 置高。

重构后不再初始化 USART3，调试输出改到 USART1；OLED、ESP8266、GPIO
和 MPU6050 的默认引脚重新分配为互不重叠的组合。如果继续使用原来的面包板
接线，需要先按上表重新接线或修改 `USER/app_config.h`。

## 配置

所有可配置项都在 [`USER/app_config.h`](USER/app_config.h)。

### 功能开关

| 宏 | 默认值 | 作用 |
|---|---:|---|
| `APP_ENABLE_OLED` | 1 | 初始化并刷新 OLED |
| `APP_ENABLE_MPU` | 1 | 初始化 MPU6050 原始数据读取 |
| `APP_ENABLE_MPU_DMP` | 1 | 尝试加载 DMP；失败时回退到加速度计角度 |
| `APP_ENABLE_BUZZER` | 1 | 初始化并驱动蜂鸣器告警 |
| `APP_MQTT_ENABLED` | 0 | 启用 ESP8266 Wi-Fi/MQTT |
| `APP_MQTT_ENABLE_SUBSCRIBE` | 1 | 订阅属性设置主题 |

默认关闭 MQTT，是为了避免在没有使用者的 Wi-Fi、阿里云设备证书时启动即
长时间等待，也避免把个人凭据提交到公开仓库。

### 启用 MQTT

1. 在 `app_config.h` 填写 Wi-Fi SSID、密码。
2. 填写阿里云 ProductKey、DeviceName、MQTT 用户名、密码和 Client ID。
3. 将 `APP_MQTT_ENABLED` 改为 1。
4. 根据 ESP-AT 固件版本确认其支持 `AT+MQTTUSERCFG`、`AT+MQTTCONN`、
   `AT+MQTTSUB` 和 `AT+MQTTPUB`。
5. 建议把真实参数放到本地未跟踪文件中，或在 `app_config.h` 修改后不要提交。

代码不会把 Wi-Fi 密码、MQTT 密码或完整连接命令打印到调试串口。

## 工作流程

```text
系统时钟
  -> TIM2 1 ms 时基
  -> USART1 调试输出
  -> ADC / GPIO / LED / 蜂鸣器
  -> OLED
  -> MPU6050 + DMP（可选）
  -> MAX30102 初始化与 PART_ID 校验
  -> MQTT 连接（可选）
  -> 每次主循环读取一个 PPG 样本
  -> 500 点后计算 HR / SpO2
  -> 串口输出 + OLED 刷新 + 异步 MQTT 发布
  -> 保留最近 400 点，再采集 100 点形成下一个窗口
```

如果 MAX30102 连续超时或 I2C 失败，程序每 2 秒重试初始化，不会卡在
`while (MAX30102_INT == 1)`。MQTT 发布采用“发送命令后立即返回”的方式，
避免服务器响应等待占用 PPG FIFO 的 100 Hz 采样周期。

## 代码模块说明

### `health_monitor.c`

整个应用的核心服务层。它负责：

- 维护 500 点红光/红外缓冲区；
- 调用 Maxim 心率和血氧算法；
- 校验输出有效性和生理范围；
- 每 1 秒更新 ADC 和数字输入；
- 每 200 ms 更新 MPU6050 姿态；
- 用有效 SpO2 和双阈值控制告警；
- 组织 OLED 屏幕内容。

主程序只处理“窗口是否完成”和“是否收到远程命令”，避免把传感器细节堆在
`main.c` 中。

### `max30102.c`

驱动完成复位、PART_ID 校验、FIFO 清空、模式配置、INT 等待和 6 字节 FIFO
读取。红光为字节 0..2，红外为字节 3..5，均截取 18 位 ADC 数据。

### `myiic.c`

MAX30102 软件 I2C。PB7/PB8 使用开漏输出，读取 SDA 时直接读取 IDR。每个
ACK 等待都有超时，所有多字节读写都返回成功/失败。

### `algorithm.c`

Maxim 参考算法：去直流、4 点滑动平均、一阶差分、Hamming 窗、峰值检测、
峰间距心率计算，以及红/红外 AC/DC 比值查表得到 SpO2。

本次修正了原算法在计算红外 AC 时误用红光峰值索引的问题，并增加输入长度
和空指针保护。算法要求输入长度严格等于 `BUFFER_SIZE = 500`。

### `mpu6050.c` / `mpuiic.c` / `eMPL`

PB10/PB11 软件 I2C 读取 MPU6050。上电后读取 WHO_AM_I，DMP 初始化成功后
输出 pitch/roll/yaw；失败时由加速度计计算静态 pitch/roll。没有磁力计，
yaw 不能作为绝对航向使用。

### `esp8266.c` / `mqtt.c`

AT 指令按 UART 空闲中断收帧，并在最后一段响应后等待 30 ms 静默再判断
ACK。所有连接过程有超时和有限重试。MQTT 健康消息使用 ASCII JSON，字段名
和逗号均合法；JSON 引号会被正确转义为 AT 字符串。

### `oled.c`

SSD1306 128x64 软件 SPI。显示内容全部使用 ASCII，便于使用现成 6x8/8x16
字库，不依赖中文点阵。

## 串口输出

USART1 参数为 `115200 8N1`。每个 5 秒窗口完成后输出一行：

```text
HEART,HR=72,SpO2=98,Pitch=1.2,Roll=-3.4,Yaw=45.0,PressureADC=1234,PressureMV=994,TempC=36.53,Input=1
```

数据无效时：

```text
HEART,HR=INVALID,SpO2=INVALID,Pitch=0.0,Roll=0.0,PressureADC=1234,PressureMV=994,Timeouts=0,Errors=0
```

`PressureADC` 是 0..4095 原始值，`PressureMV` 是按 3.3 V 参考电压计算的
电压值。它还不是物理压力单位，因为原工程没有传感器型号、量程、零点和
标定数据。

## MQTT 数据格式

启用 MQTT 后，属性上报的有效 JSON 结构如下：

```json
{
  "id": "123456",
  "version": "1.0",
  "params": {
    "heartRate": 72,
    "heartRateValid": 1,
    "spo2": 98,
    "spo2Valid": 1,
    "pressureAdc": 1234,
    "pressureMv": 994,
    "pitchX100": 120,
    "rollX100": -340,
    "yawX100": 4500,
    "temperatureCentiC": 3653,
    "digitalInput": 1,
    "alarm": 0,
    "powerSwitch": 1
  }
}
```

为减少浮点格式差异，姿态角度乘以 100 后传输，字段名带 `X100`。

## 构建

1. 使用 Keil MDK 5 打开 `USER/IIC.uvprojx`。
2. 目标器件为 `STM32F103C8`，编译器为 ARMCC 5.06。
3. 选择 `Rebuild`，输出位于 `OBJ/IIC.hex` 和 `OBJ/IIC.axf`。

命令行示例：

```powershell
& "C:\Keil_v5\UV4\UV4.exe" -b ".\USER\IIC.uvprojx" -j0 -o ".\OBJ\build.log"
```

`OBJ/` 已被 `.gitignore` 排除，干净仓库可以直接用 Keil 重建。

## 原工程存在的主要问题

### 运行逻辑

1. MAX30102 INT 等待没有超时，传感器掉线后会永久卡死。
2. MPU6050 初始化放在测量循环的 `while` 内，失败时每 5 秒重复尝试并打断
   PPG 流程。
3. 第一次采样结束后执行 `un_prev_data = aun_red_buffer[i]`，此时 `i` 已等于
   500，发生数组越界。
4. 曲线缩放直接计算 `(max-min)/20`，信号平坦时会除零。
5. 心率有效但 SpO2 无效时仍把 SpO2 显示为有效；有效标志组合错误。
6. MQTT 在每个样本后都同步发送，服务器响应和硬编码延时严重破坏 100 Hz
   时序。
7. 主循环中存在每轮 500 ms 固定延时和不必要的 `mpu_dmp_init()` 重试。

### 数据与协议

1. MQTT JSON 使用了全角中文逗号，标准 JSON 解析必然失败。
2. `Sp02`、`Tmep1`、`waterpruess` 等字段名拼写错误。
3. MQTT JSON 本地缓冲区为 1024 字节，接近/超过小型 Cortex-M3 默认栈容量。
4. ADC 电压写成 `ad1 * 0.0806`，比 3.3 V/4095 的正确系数小 10 倍。
5. 把 ADC 显示字符串再次 `atoi()` 当压力值，解析出的数字没有明确单位和
   有效范围。
6. 心率、SpO2 驱动错误值 `-999` 可以被后续阈值条件误用。

### I2C 与驱动

1. MAX30102 初始化不读取 PART_ID，不确认实际器件是否存在。
2. MAX30102 复位后不等待 RESET 位清零。
3. 几乎所有软件 I2C 写操作忽略 ACK；总线故障仍可能继续执行。
4. MPU6050 连读函数在寄存器地址和读地址阶段忽略 NACK。
5. DMP 所需的 `get_ms()` 是空函数，时间戳无效。
6. 原工程同时存在多套 USART 初始化函数和重复的 `fputc()` 定义。

### 引脚与安全

1. MPU6050 和 USART3 共用 PB10/PB11。
2. OLED、ESP8266、传感器和蜂鸣器存在多处重叠。
3. Wi-Fi 密码、阿里云设备用户名、密码和域名被硬编码在公开头文件中。
4. 原代码没有对网络连接结果统一判断，失败时存在无限重连。

## 重构后的已知限制

- 目前只完成编译和静态路径检查，没有在对应实物板、真实手指和真实阿里云
  设备上做端到端验证。
- SpO2 算法来自 Maxim 参考实现，适合教学与原型验证，不能作为医疗器械。
- 压力通道只输出 ADC 和电压，没有换算成 kPa/MPa，因为缺少传感器标定参数。
- 运行期 MQTT 发布是非阻塞式，不检查每一条消息的服务器 ACK。连续断网时
  需要重新上电或后续增加连接状态机。
- 依赖 ESP-AT 的 MQTT 命令集，AT 固件版本不同可能返回不同文本。
- DMP 的 yaw 会漂移，并且 MPU6050 没有磁力计，不能提供绝对航向。
- 当前没有启用独立看门狗。联网初始化可能耗时较长，若直接加入看门狗需要在
  所有长等待路径中合理喂狗。
- MAX30102 LED 电流当前约 7 mA，不同手指、外壳和电源条件下可能需要重新
  调整 `APP_MAX_LED_CURRENT_REG`。

## 安全提醒

原工程源码中曾包含 Wi-Fi 密码、阿里云设备凭据和服务器地址。它们没有进入
本次重构仓库，但已经暴露过的凭据应立即在路由器/阿里云控制台轮换。用于创建
仓库的 GitHub Classic PAT 也建议在推送完成后立即撤销并重新生成最小权限令牌。

## 第三方代码

STM32 标准外设库、CMSIS、Maxim PPG 算法和 InvenSense eMPL 代码保留了
各自文件中的版权/许可声明。公开或商业分发前请核对全部第三方许可，不对
本项目自动授予与原始第三方代码冲突的许可证。详见
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。
