#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <stdint.h>
#include "app_config.h"

/* HealthMonitor_Update 的返回值。 */
enum
{
    HEALTH_UPDATE_NONE = 0,
    HEALTH_UPDATE_WINDOW_READY = 1,
    HEALTH_UPDATE_MAX_ERROR = -1
};

/*
 * 一次完整健康快照。
 * 时间字段使用系统启动后的毫秒数；浮点姿态单位均为度。
 * 有效标志必须与数值一起使用，避免把驱动错误值当成测量值。
 */
typedef struct
{
    int32_t heart_rate_bpm;
    uint8_t heart_rate_valid;
    int32_t spo2_percent;
    uint8_t spo2_valid;

    uint16_t pressure_adc;
    uint16_t pressure_mv;
    uint8_t digital_input;

    float pitch_deg;
    float roll_deg;
    float yaw_deg;
    int16_t temperature_centi_c;
    uint8_t motion_valid;

    uint8_t alarm_active;
    uint8_t remote_output;
    uint32_t timestamp_ms;
    uint32_t ppg_samples;
    uint16_t ppg_timeouts;
    uint16_t ppg_errors;
} HealthData_t;

/*
 * 初始化所有传感器。MAX30102 是核心传感器，初始化失败返回非 0；
 * MPU6050、OLED 等辅助设备失败不会阻止主程序运行。
 */
int HealthMonitor_Init(void);

/*
 * 采集一个 MAX30102 样本并顺带执行低频辅助传感器任务。
 * 返回 HEALTH_UPDATE_WINDOW_READY 时，HealthData_t 中的结果已经更新。
 * 调用者完成显示/上报后必须调用 HealthMonitor_ContinueWindow()。
 */
int HealthMonitor_Update(void);
void HealthMonitor_ContinueWindow(void);
void HealthMonitor_RestartPpg(void);

const HealthData_t *HealthMonitor_GetData(void);
const uint32_t *HealthMonitor_GetRedBuffer(void);
const uint32_t *HealthMonitor_GetIrBuffer(void);

/* 远程开关只控制板载状态灯，不改变健康数据有效性。 */
void HealthMonitor_SetRemoteOutput(uint8_t enabled);
uint8_t HealthMonitor_GetRemoteOutput(void);

/* OLED 可用时刷新一屏 ASCII 状态；不可用时安全返回。 */
void HealthMonitor_Display(void);

#endif /* HEALTH_MONITOR_H */
