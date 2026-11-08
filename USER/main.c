/*
 * MAX30102 STM32 健康监测节点
 * ----------------------------
 * 主循环只负责初始化和事件调度：
 *   1. 初始化 TIM2、USART1、传感器、OLED 和可选 MQTT；
 *   2. 每次循环采集一个 PPG 样本；
 *   3. 5 秒窗口完成时计算 HR/SpO2，刷新显示并异步上报；
 *   4. 解析 MQTT PowerSwitch 命令，控制板载状态灯。
 *
 * MAX30102 初始化失败时主程序不会死循环，而是每 2 秒重试一次并打印错误码。
 */

#include "app_config.h"
#include "delay.h"
#include "health_monitor.h"
#include "mqtt.h"
#include "stm32f10x.h"
#include "timebase.h"
#include "usart.h"

#include <stdio.h>

static void report_window(const HealthData_t *data)
{
    if (data->heart_rate_valid)
    {
        printf("HEART,HR=%ld,SpO2=%ld,Pitch=%.1f,Roll=%.1f,Yaw=%.1f,"
               "PressureADC=%u,PressureMV=%u,TempC=%.2f,Input=%u\r\n",
               (long)data->heart_rate_bpm,
               (long)data->spo2_percent,
               data->pitch_deg,
               data->roll_deg,
               data->yaw_deg,
               (unsigned int)data->pressure_adc,
               (unsigned int)data->pressure_mv,
               ((float)data->temperature_centi_c) / 100.0f,
               (unsigned int)data->digital_input);
    }
    else
    {
        printf("HEART,HR=INVALID,SpO2=INVALID,Pitch=%.1f,Roll=%.1f,"
               "PressureADC=%u,PressureMV=%u,Timeouts=%u,Errors=%u\r\n",
               data->pitch_deg,
               data->roll_deg,
               (unsigned int)data->pressure_adc,
               (unsigned int)data->pressure_mv,
               (unsigned int)data->ppg_timeouts,
               (unsigned int)data->ppg_errors);
    }
}

int main(void)
{
    int init_result;
    int update_result;
#if APP_MQTT_ENABLED && APP_MQTT_ENABLE_SUBSCRIBE
    int remote_state;
#endif
    const HealthData_t *health;

    SystemInit();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Timebase_Init();
    delay_init();
    uart_init(APP_DEBUG_BAUDRATE);

    printf("\r\nMAX30102 health monitor starting\r\n");
    printf("Pin map: MAX30102 PB7/PB8/INT PB9, debug USART1 PA9/PA10\r\n");

    init_result = HealthMonitor_Init();
    if (init_result != 0)
    {
        printf("MAX30102 init failed: %d. Retry every 2 s.\r\n", init_result);
    }

#if APP_MQTT_ENABLED
    /* 联网较慢，放在 PPG 采样开始前，避免打乱 5 秒分析窗口。 */
    if (MQTT_Connect() == 0U)
    {
        printf("MQTT connection failed; local measurement continues.\r\n");
    }
#endif

    while (1)
    {
        update_result = HealthMonitor_Update();

        if (update_result == HEALTH_UPDATE_MAX_ERROR)
        {
            /*
             * 传感器掉线时先保持系统存活并周期性重试，不进入无超时的
             * while(MAX30102_INT==1) 死循环。
             */
            printf("MAX30102 unavailable; retrying...\r\n");
            delay_ms(2000U);
            HealthMonitor_RestartPpg();
            continue;
        }

        if (update_result == HEALTH_UPDATE_WINDOW_READY)
        {
            health = HealthMonitor_GetData();
            report_window(health);
            HealthMonitor_Display();

#if APP_MQTT_ENABLED
            /*
             * 发布函数只发送 AT 命令，不等待 MQTT 服务器应答。
             * 因此单次上报只阻塞串口写入，不会让 PPG FIFO 持续溢出。
             */
            (void)MQTT_PublishHealth(health);
#endif
            HealthMonitor_ContinueWindow();
        }

#if APP_MQTT_ENABLED && APP_MQTT_ENABLE_SUBSCRIBE
        remote_state = MQTT_PollPowerSwitch();
        if (remote_state >= 0)
        {
            HealthMonitor_SetRemoteOutput((uint8_t)remote_state);
            printf("Remote output set to %d\r\n", remote_state);
        }
#endif
    }
}
