#include "health_monitor.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "algorithm.h"
#include "bee.h"
#include "delay.h"
#include "input.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "led.h"
#include "max30102.h"
#include "mpu6050.h"
#include "oled.h"
#include "timebase.h"

#if (APP_PPG_WINDOW_SAMPLES != BUFFER_SIZE)
#error "MAX30102 sample window must match BUFFER_SIZE in algorithm.h"
#endif

static uint32_t s_red_buffer[BUFFER_SIZE];
static uint32_t s_ir_buffer[BUFFER_SIZE];
static HealthData_t s_health;

static uint16_t s_sample_count;
static uint16_t s_consecutive_timeouts;
static uint8_t s_max_ready;
static uint8_t s_oled_ready;
static uint8_t s_mpu_ready;
static uint8_t s_dmp_ready;

static uint32_t s_last_aux_ms;
static uint32_t s_last_motion_ms;

/* 读取 ADC、数字输入和状态灯。频率较低，不占用 PPG 采样路径。 */
static void update_auxiliary(void)
{
    uint32_t adc_sum;
    uint32_t now;

    now = Timebase_GetMillis();
    if (!Timebase_Elapsed(s_last_aux_ms, APP_AUX_UPDATE_INTERVAL_MS))
    {
        return;
    }
    s_last_aux_ms = now;

    s_health.pressure_adc = Get_Adc_Average(APP_PRESSURE_ADC_CHANNEL, 8U);
    adc_sum = (uint32_t)s_health.pressure_adc * APP_PRESSURE_VREF_MV;
    s_health.pressure_mv = (uint16_t)((adc_sum + 2047U) / 4095U);
    s_health.digital_input = SENSOR_Read();

    LED_Set(s_health.remote_output ? 1U : 0U);
}

/*
 * MPU6050 更新：
 * 优先读取 DMP 四元数；DMP 不可用时用加速度计计算静态 pitch/roll。
 * 无磁力计，因此无 DMP 时 yaw 无法可靠计算，明确保持为 0。
 */
static void update_motion(void)
{
    short ax;
    short ay;
    short az;
    float axf;
    float ayf;
    float azf;

    if (!s_mpu_ready)
    {
        return;
    }
    if (!Timebase_Elapsed(s_last_motion_ms, APP_MPU_UPDATE_INTERVAL_MS))
    {
        return;
    }
    s_last_motion_ms = Timebase_GetMillis();

    if (s_dmp_ready && mpu_dmp_get_data(&s_health.pitch_deg,
                                        &s_health.roll_deg,
                                        &s_health.yaw_deg) == 0U)
    {
        s_health.motion_valid = 1U;
    }
    else if (MPU_Get_Accelerometer(&ax, &ay, &az) == 0U)
    {
        axf = (float)ax;
        ayf = (float)ay;
        azf = (float)az;
        s_health.pitch_deg = atan2f(-axf, sqrtf(ayf * ayf + azf * azf)) * 57.29578f;
        s_health.roll_deg = atan2f(ayf, azf) * 57.29578f;
        s_health.yaw_deg = 0.0f;
        s_health.motion_valid = 1U;
    }

    s_health.temperature_centi_c = MPU_Get_Temperature();
}

/* 使用回差和有效标志控制蜂鸣器，避免无效结果触发误报。 */
static void update_alarm(void)
{
    uint8_t spo2_alarm;
    uint8_t pressure_alarm;

    spo2_alarm = 0U;
    if (s_health.spo2_valid)
    {
        if ((s_health.spo2_percent <= APP_ALARM_SPO2_ON) ||
            (s_health.alarm_active && s_health.spo2_percent < APP_ALARM_SPO2_OFF))
        {
            spo2_alarm = 1U;
        }
    }

    pressure_alarm = 0U;
    if (APP_ALARM_PRESSURE_MV > 0U &&
        s_health.pressure_mv >= APP_ALARM_PRESSURE_MV)
    {
        pressure_alarm = 1U;
    }

    s_health.alarm_active = (uint8_t)(spo2_alarm || pressure_alarm);
#if APP_ENABLE_BUZZER
    BEEP_Set(s_health.alarm_active);
#endif
}

static void invalidate_result(void)
{
    s_health.heart_rate_bpm = 0;
    s_health.heart_rate_valid = 0U;
    s_health.spo2_percent = 0;
    s_health.spo2_valid = 0U;
}

int HealthMonitor_Init(void)
{
    int max_result;

    memset(&s_health, 0, sizeof(s_health));
    memset(s_red_buffer, 0, sizeof(s_red_buffer));
    memset(s_ir_buffer, 0, sizeof(s_ir_buffer));

    s_sample_count = 0U;
    s_consecutive_timeouts = 0U;
    s_health.remote_output = 1U;
    s_last_aux_ms = Timebase_GetMillis();
    s_last_motion_ms = s_last_aux_ms;

    LED_Init();
#if APP_ENABLE_BUZZER
    BEEP_Init();
#endif
    SENSOR_Init();
    Adc_Init();

    s_oled_ready = 0U;
#if APP_ENABLE_OLED
    OLED_Init();
    s_oled_ready = 1U;
#endif

    s_mpu_ready = 0U;
    s_dmp_ready = 0U;
#if APP_ENABLE_MPU
    if (MPU_Init() == 0U)
    {
        s_mpu_ready = 1U;
#if APP_ENABLE_MPU_DMP
        if (mpu_dmp_init() == 0U)
        {
            s_dmp_ready = 1U;
        }
#endif
    }
#endif

    max_result = max30102_init();
    if (max_result == MAX30102_OK)
    {
        s_max_ready = 1U;
        return 0;
    }

    s_max_ready = 0U;
    return max_result;
}

int HealthMonitor_Update(void)
{
    max30102_sample_t sample;
    int sample_result;
    int8_t spo2_valid;
    int8_t hr_valid;
    int32_t spo2;
    int32_t heart_rate;

    update_auxiliary();
    update_motion();
    update_alarm();

    if (!s_max_ready)
    {
        return HEALTH_UPDATE_MAX_ERROR;
    }
    if (s_sample_count >= BUFFER_SIZE)
    {
        return HEALTH_UPDATE_NONE;
    }

    sample_result = max30102_wait_for_sample(APP_MAX_TIMEOUT_MS);
    if (sample_result != MAX30102_OK)
    {
        s_consecutive_timeouts++;
        s_health.ppg_timeouts++;
        if (s_consecutive_timeouts >= APP_MAX_MAX_CONSECUTIVE_FAILS)
        {
            s_max_ready = 0U;
            invalidate_result();
            return HEALTH_UPDATE_MAX_ERROR;
        }
        return HEALTH_UPDATE_NONE;
    }

    sample_result = max30102_read_sample(&sample);
    if (sample_result != MAX30102_OK)
    {
        s_health.ppg_errors++;
        s_consecutive_timeouts++;
        if (s_consecutive_timeouts >= APP_MAX_MAX_CONSECUTIVE_FAILS)
        {
            s_max_ready = 0U;
            invalidate_result();
            return HEALTH_UPDATE_MAX_ERROR;
        }
        return HEALTH_UPDATE_NONE;
    }

    s_consecutive_timeouts = 0U;
    s_red_buffer[s_sample_count] = sample.red;
    s_ir_buffer[s_sample_count] = sample.ir;
    s_sample_count++;
    s_health.ppg_samples = s_sample_count;

    if (s_sample_count < BUFFER_SIZE)
    {
        return HEALTH_UPDATE_NONE;
    }

    spo2 = 0;
    heart_rate = 0;
    spo2_valid = 0;
    hr_valid = 0;
    maxim_heart_rate_and_oxygen_saturation(s_ir_buffer,
                                           (int32_t)BUFFER_SIZE,
                                           s_red_buffer,
                                           &spo2,
                                           &spo2_valid,
                                           &heart_rate,
                                           &hr_valid);

    /* 对算法输出再校验一次生理范围，避免把异常峰误判为有效结果。 */
    s_health.heart_rate_valid =
        (uint8_t)((hr_valid == 1) && (heart_rate >= 30) && (heart_rate <= 220));
    s_health.spo2_valid =
        (uint8_t)((spo2_valid == 1) && (spo2 >= 50) && (spo2 <= 100));

    s_health.heart_rate_bpm = s_health.heart_rate_valid ? heart_rate : 0;
    s_health.spo2_percent = s_health.spo2_valid ? spo2 : 0;
    s_health.timestamp_ms = Timebase_GetMillis();
    update_alarm();
    return HEALTH_UPDATE_WINDOW_READY;
}

void HealthMonitor_ContinueWindow(void)
{
    uint16_t i;
    const uint16_t keep = BUFFER_SIZE - (uint16_t)APP_MAX_SAMPLE_RATE_HZ;

    if (s_sample_count != BUFFER_SIZE)
    {
        return;
    }

    for (i = 0U; i < keep; i++)
    {
        s_red_buffer[i] = s_red_buffer[i + APP_MAX_SAMPLE_RATE_HZ];
        s_ir_buffer[i] = s_ir_buffer[i + APP_MAX_SAMPLE_RATE_HZ];
    }
    s_sample_count = keep;
    s_health.ppg_samples = s_sample_count;
}

void HealthMonitor_RestartPpg(void)
{
    s_sample_count = 0U;
    s_consecutive_timeouts = 0U;
    s_health.ppg_samples = 0U;
    invalidate_result();
    if (max30102_init() == MAX30102_OK)
    {
        s_max_ready = 1U;
    }
}

const HealthData_t *HealthMonitor_GetData(void)
{
    return &s_health;
}

const uint32_t *HealthMonitor_GetRedBuffer(void)
{
    return s_red_buffer;
}

const uint32_t *HealthMonitor_GetIrBuffer(void)
{
    return s_ir_buffer;
}

void HealthMonitor_SetRemoteOutput(uint8_t enabled)
{
    s_health.remote_output = enabled ? 1U : 0U;
    LED_Set(s_health.remote_output);
}

uint8_t HealthMonitor_GetRemoteOutput(void)
{
    return s_health.remote_output;
}

void HealthMonitor_Display(void)
{
    char line[24];

#if APP_ENABLE_OLED
    if (!s_oled_ready)
    {
        return;
    }

    OLED_ClearBuffer();

    if (s_health.heart_rate_valid)
    {
        sprintf(line, "HR:%3ld SpO2:%2ld%%",
                (long)s_health.heart_rate_bpm,
                (long)s_health.spo2_percent);
    }
    else
    {
        sprintf(line, "HR:--- SpO2:---");
    }
    OLED_ShowString(0U, 0U, (const uint8_t *)line, 16U);

    sprintf(line, "P:%4u %4umV",
            (unsigned int)s_health.pressure_adc,
            (unsigned int)s_health.pressure_mv);
    OLED_ShowString(0U, 16U, (const uint8_t *)line, 16U);

    sprintf(line, "T:%2d.%01d C  IN:%u",
            (int)(s_health.temperature_centi_c / 100),
            (int)((s_health.temperature_centi_c < 0 ?
                  -s_health.temperature_centi_c : s_health.temperature_centi_c) % 100) / 10,
            (unsigned int)s_health.digital_input);
    OLED_ShowString(0U, 32U, (const uint8_t *)line, 16U);

    if (s_health.motion_valid)
    {
        sprintf(line, "%3d,%3d,%3d",
                (int)s_health.pitch_deg,
                (int)s_health.roll_deg,
                (int)s_health.yaw_deg);
    }
    else
    {
        sprintf(line, "MOTION: ---");
    }
    OLED_ShowString(0U, 48U, (const uint8_t *)line, 16U);
    OLED_Refresh_Gram();
#endif
}
