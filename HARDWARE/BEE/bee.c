#include "bee.h"

#include "app_config.h"
#include "stm32f10x.h"

void BEEP_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin = APP_BUZZER_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(APP_BUZZER_GPIO, &gpio);
    BEEP_Set(0U);
}

void BEEP_Set(uint8_t enabled)
{
    if (enabled)
    {
        GPIO_SetBits(APP_BUZZER_GPIO, APP_BUZZER_PIN);
    }
    else
    {
        GPIO_ResetBits(APP_BUZZER_GPIO, APP_BUZZER_PIN);
    }
}
