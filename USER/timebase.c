#include "timebase.h"
#include "stm32f10x.h"
#include "stm32f10x_tim.h"

static volatile uint32_t s_system_ms = 0;

void Timebase_Init(void)
{
    NVIC_InitTypeDef nvic;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* 72 MHz / 72 = 1 MHz，计数 1000 次产生 1 ms 更新中断。 */
    TIM2->CR1 = 0;
    TIM2->PSC = (SystemCoreClock / 1000000U) - 1U;
    TIM2->ARR = 999U;
    TIM2->CNT = 0U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    TIM2->DIER = TIM_DIER_UIE;

    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 3;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    s_system_ms = 0U;
    TIM2->CR1 = TIM_CR1_CEN;
}

uint32_t Timebase_GetMillis(void)
{
    /* Cortex-M3 对 32 位对齐变量执行单次 LDR，读取本身是原子的。 */
    return s_system_ms;
}

uint8_t Timebase_Elapsed(uint32_t start_ms, uint32_t interval_ms)
{
    return ((Timebase_GetMillis() - start_ms) >= interval_ms) ? 1U : 0U;
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        s_system_ms++;
    }
}
