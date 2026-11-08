#include "adc.h"

#include "app_config.h"
#include "delay.h"
#include "timebase.h"

void Adc_Init(void)
{
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

    /* 模拟输入需要关闭数字输入缓冲。 */
    gpio.GPIO_Pin = APP_PRESSURE_ADC_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(APP_PRESSURE_ADC_GPIO, &gpio);

    /* 72 MHz / 6 = 12 MHz，低于 STM32F1 ADC 的 14 MHz 上限。 */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    ADC_DeInit(ADC1);
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1U;
    ADC_Init(ADC1, &adc);

    /* 239.5 周期采样时间适合高阻抗压力传感器输出。 */
    ADC_RegularChannelConfig(ADC1,
                             APP_PRESSURE_ADC_CHANNEL,
                             1U,
                             ADC_SampleTime_239Cycles5);
    ADC_Cmd(ADC1, ENABLE);

    /* 上电后执行一次 RSTCAL -> CAL，否则首轮读数的误差会偏大。 */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) != RESET)
    {
    }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) != RESET)
    {
    }
}

uint16_t Get_Adc(uint8_t channel)
{
    uint32_t start;

    ADC_RegularChannelConfig(ADC1, channel, 1U, ADC_SampleTime_239Cycles5);
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    start = Timebase_GetMillis();
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
    {
        if (Timebase_Elapsed(start, 10U))
        {
            return 0U;      /* 超时返回 0，调用者可通过 range/valid 字段判断 */
        }
    }
    return (uint16_t)ADC_GetConversionValue(ADC1);
}

uint16_t Get_Adc_Average(uint8_t channel, uint8_t times)
{
    uint32_t sum;
    uint8_t i;

    if (times == 0U)
    {
        return 0U;
    }

    sum = 0U;
    for (i = 0U; i < times; i++)
    {
        sum += Get_Adc(channel);
        delay_us(20U);
    }
    return (uint16_t)(sum / times);
}
