#include "delay.h"

#include "stm32f10x.h"

static uint32_t s_ticks_per_us;

void delay_init(void)
{
    /* SysTick 时钟固定为 HCLK/8；72 MHz 时每微秒 9 个计数。 */
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
    s_ticks_per_us = SystemCoreClock / 8000000U;
}

void delay_us(uint32_t us)
{
    uint32_t ticks;
    uint32_t chunk_us;
    uint32_t max_chunk_us;

    if ((us == 0U) || (s_ticks_per_us == 0U))
    {
        return;
    }

    /* SysTick 是 24 位计数器，较长延时分段执行，避免 LOAD 溢出。 */
    max_chunk_us = SysTick_LOAD_RELOAD_Msk / s_ticks_per_us;
    while (us > 0U)
    {
        chunk_us = (us > max_chunk_us) ? max_chunk_us : us;
        ticks = chunk_us * s_ticks_per_us;
        if (ticks == 0U)
        {
            ticks = 1U;
        }

        SysTick->LOAD = ticks - 1U;
        SysTick->VAL = 0U;
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U)
        {
        }
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
        SysTick->VAL = 0U;
        us -= chunk_us;
    }
}

void delay_ms(uint32_t ms)
{
    while (ms > 0U)
    {
        delay_us(1000U);
        ms--;
    }
}
