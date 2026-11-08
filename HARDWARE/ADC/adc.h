#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "stm32f10x.h"
#include "stm32f10x_adc.h"

/*
 * 12 位 ADC1 单通道采样。
 * 当前用于 PA1/ADC1_IN1，只返回原始码值；电压换算由调用者完成。
 */
void Adc_Init(void);
uint16_t Get_Adc(uint8_t channel);
uint16_t Get_Adc_Average(uint8_t channel, uint8_t times);

#define ADC_ADC1_CHANNEL(channel) (channel)

#endif /* ADC_H */
