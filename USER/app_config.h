#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "stm32f10x.h"

/*
 * 集中配置入口
 * ----------------
 * 本文件只放“可能与具体硬件或使用场景有关”的配置：
 * 1. 功能开关；
 * 2. GPIO 引脚映射；
 * 3. 告警阈值；
 * 4. MQTT 连接参数。
 *
 * 修改引脚前请先核对原理图。下面给出的默认映射避开 USART1、USART2、
 * MAX30102 和 MPU6050 之间的冲突。默认值并不保证与原工程面包板上的
 * 实际接线一致，详见 README 的“硬件接线”章节。
 */

/* ---------------- 功能开关 ---------------- */
#define APP_ENABLE_OLED             1   /* SSD1306 128x64，4 线 SPI */
#define APP_ENABLE_MPU              1   /* MPU6050 原始数据读取 */
#define APP_ENABLE_MPU_DMP          1   /* DMP 姿态解算；失败时仍可用加速度计估算 */
#define APP_ENABLE_BUZZER           1   /* 本地低血氧告警 */
#define APP_MQTT_ENABLED            0   /* 默认关闭，避免把真实凭据提交到公开仓库 */
#define APP_MQTT_ENABLE_SUBSCRIBE   1   /* MQTT 已启用时订阅控制主题 */

/* ---------------- 调试串口 ---------------- */
/* USART1 TX=PA9，RX=PA10，115200-8-N-1。 */
#define APP_DEBUG_BAUDRATE          115200UL

/* ---------------- MAX30102 ---------------- */
/*
 * 软件 I2C：SCL=PB7，SDA=PB8，INT=PB9。
 * PB7/PB8 需要 4.7 kOhm 左右上拉；MAX30102 模块通常已带 I2C 上拉。
 */
#define APP_MAX_I2C_GPIO            GPIOB
#define APP_MAX_I2C_SCL_PIN         GPIO_Pin_7
#define APP_MAX_I2C_SDA_PIN         GPIO_Pin_8
#define APP_MAX_INT_GPIO            GPIOB
#define APP_MAX_INT_PIN             GPIO_Pin_9
#define APP_I2C_DELAY_US            2U

/*
 * MAX30102 配置：
 * - SpO2 模式（红光 + 红外）；
 * - 100 Hz 输出，5 秒形成 500 点窗口；
 * - 18-bit ADC，411 us 脉宽，4096 nA 量程；
 * - LED 电流约 7 mA，可按实际佩戴信号强度调整。
 */
#define APP_MAX_SAMPLE_RATE_HZ      100
#define APP_PPG_WINDOW_SAMPLES      (5U * APP_MAX_SAMPLE_RATE_HZ)
#define APP_MAX_LED_CURRENT_REG     0x24U
#define APP_MAX_TIMEOUT_MS          50U
#define APP_MAX_MAX_CONSECUTIVE_FAILS 10U

/* ---------------- 模拟输入和数字输入 ---------------- */
/* PA1：ADC1_IN1，12 位、0..3.3 V。 */
#define APP_PRESSURE_ADC_GPIO       GPIOA
#define APP_PRESSURE_ADC_PIN        GPIO_Pin_1
#define APP_PRESSURE_ADC_CHANNEL    ADC_Channel_1
#define APP_PRESSURE_VREF_MV        3300U

/* PA5：外部数字传感器，低有效；内部上拉。 */
#define APP_SENSOR_GPIO             GPIOA
#define APP_SENSOR_PIN              GPIO_Pin_5

/* ---------------- OLED ---------------- */
/*
 * 原工程的 OLED 引脚与 USART2/ESP8266、传感器和蜂鸣器重叠。
 * 新映射：
 * CS=PB12，RST=PB13，DC/RS=PB14，SCLK=PB15，SDIN=PA8。
 */
#define APP_OLED_CS_PORT            GPIOB
#define APP_OLED_CS_PIN             GPIO_Pin_12
#define APP_OLED_RST_PORT           GPIOB
#define APP_OLED_RST_PIN            GPIO_Pin_13
#define APP_OLED_DC_PORT            GPIOB
#define APP_OLED_DC_PIN             GPIO_Pin_14
#define APP_OLED_SCLK_PORT          GPIOB
#define APP_OLED_SCLK_PIN           GPIO_Pin_15
#define APP_OLED_SDIN_PORT          GPIOA
#define APP_OLED_SDIN_PIN           GPIO_Pin_8

/* ---------------- MPU6050 ---------------- */
/*
 * 软件 I2C：SCL=PB10，SDA=PB11；AD0=PA15（低电平，地址 0x68）。
 * 原工程同时把 PB10/PB11 用作 USART3，导致传感器和串口互相覆盖。
 * 现在调试日志改走 USART1，不再初始化 USART3。
 */
#define APP_MPU_I2C_GPIO            GPIOB
#define APP_MPU_I2C_SCL_PIN         GPIO_Pin_10
#define APP_MPU_I2C_SDA_PIN         GPIO_Pin_11
#define APP_MPU_AD0_GPIO            GPIOA
#define APP_MPU_AD0_PIN             GPIO_Pin_15
#define APP_MPU_UPDATE_INTERVAL_MS  200U

/* ---------------- LED、蜂鸣器和告警 ---------------- */
/* STM32F103C8 最小系统板常见板载 LED：PC13，低电平点亮。 */
#define APP_STATUS_LED_GPIO         GPIOC
#define APP_STATUS_LED_PIN          GPIO_Pin_13
#define APP_STATUS_LED_ACTIVE_LOW   1

/* PA7：有源蜂鸣器，高电平响。如果模块低电平触发，请修改 bee.c。 */
#define APP_BUZZER_GPIO             GPIOA
#define APP_BUZZER_PIN              GPIO_Pin_7

/*
 * 告警使用有效值，避免把 -999/无效结果当成低血氧。
 * 低于 APP_ALARM_SPO2_ON 时开启；恢复到 APP_ALARM_SPO2_OFF 后关闭，
 * 两个阈值形成回差，防止结果在边界附近反复鸣叫。
 */
#define APP_ALARM_SPO2_ON           90
#define APP_ALARM_SPO2_OFF          93

/*
 * 压力通道只提供电压/ADC 原始值，不假设传感器量程和零点。
 * 设置非 0 阈值后，压力也参与告警；默认 0 表示关闭该项。
 */
#define APP_ALARM_PRESSURE_MV       0U

/* ---------------- ESP8266 / MQTT ---------------- */
/* USART2 TX=PA2，RX=PA3；RST=PA4；CH_PD=PB5。 */
#define APP_ESP8266_BAUDRATE        115200UL
#define APP_ESP8266_RST_GPIO        GPIOA
#define APP_ESP8266_RST_PIN         GPIO_Pin_4
#define APP_ESP8266_CHPD_GPIO       GPIOB
#define APP_ESP8266_CHPD_PIN        GPIO_Pin_5
#define APP_ESP8266_RX_BUFFER_SIZE  512U

/*
 * 请复制真实参数到本地后再编译，不要把密码和个人 Token 推到公开仓库。
 * 阿里云示例：
 *   HOST: <ProductKey>.iot-as-mqtt.cn-shanghai.aliyuncs.com
 *   CLIENT_ID: <DeviceName>|securemode=3,signmethod=hmacsha1|
 *   USERNAME: <DeviceName>&<ProductKey>
 */
#define APP_WIFI_SSID               "YOUR_WIFI_SSID"
#define APP_WIFI_PASSWORD           "YOUR_WIFI_PASSWORD"
#define APP_MQTT_HOST               "YOUR_PRODUCT_KEY.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define APP_MQTT_PORT               1883
#define APP_MQTT_CLIENT_ID          "YOUR_DEVICE_NAME|securemode=3,signmethod=hmacsha1|"
#define APP_MQTT_USERNAME           "YOUR_DEVICE_NAME&YOUR_PRODUCT_KEY"
#define APP_MQTT_PASSWORD           "YOUR_MQTT_PASSWORD"
#define APP_MQTT_TOPIC_PROPERTY_POST "/sys/YOUR_PRODUCT_KEY/YOUR_DEVICE_NAME/thing/event/property/post"
#define APP_MQTT_TOPIC_PROPERTY_SET  "/sys/YOUR_PRODUCT_KEY/YOUR_DEVICE_NAME/thing/service/property/set"

/* 运行时刷新/上报周期，PPG 分析窗口本身固定为 5 秒。 */
#define APP_AUX_UPDATE_INTERVAL_MS  1000U
#define APP_MQTT_POLL_INTERVAL_MS   250U

#endif /* APP_CONFIG_H */
