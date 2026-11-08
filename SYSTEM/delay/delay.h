#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

/*
 * SysTick 忙等待延时。
 * TIM2 负责系统毫秒时间；SysTick 只服务于 I2C、ADC、OLED 等短延时。
 */
void delay_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif /* DELAY_H */
