#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

/*
 * TIM2 1 ms 系统时基。
 * SysTick 已被 delay_us/delay_ms 占用；TIM2 只负责累计毫秒时间，用于
 * 非阻塞调度和所有驱动超时。不要在 TIM2 中断中执行 I2C、串口输出等耗时操作。
 */
void Timebase_Init(void);
uint32_t Timebase_GetMillis(void);

/*
 * 无符号减法可安全处理 32 位毫秒计数回绕。
 * 返回值：1=已到达，0=未到达。
 */
uint8_t Timebase_Elapsed(uint32_t start_ms, uint32_t interval_ms);

#endif /* TIMEBASE_H */
