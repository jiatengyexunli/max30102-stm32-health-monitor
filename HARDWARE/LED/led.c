#include "led.h"

#include "app_config.h"
#include "stm32f10x.h"

void LED_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    gpio.GPIO_Pin = APP_STATUS_LED_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(APP_STATUS_LED_GPIO, &gpio);
    LED_Set(0U);
}

void LED_Set(uint8_t enabled)
{
#if APP_STATUS_LED_ACTIVE_LOW
    if (enabled)
    {
        GPIO_ResetBits(APP_STATUS_LED_GPIO, APP_STATUS_LED_PIN);
    }
    else
    {
        GPIO_SetBits(APP_STATUS_LED_GPIO, APP_STATUS_LED_PIN);
    }
#else
    if (enabled)
    {
        GPIO_SetBits(APP_STATUS_LED_GPIO, APP_STATUS_LED_PIN);
    }
    else
    {
        GPIO_ResetBits(APP_STATUS_LED_GPIO, APP_STATUS_LED_PIN);
    }
#endif
}
