#include "input.h"

void SENSOR_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin = APP_SENSOR_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(APP_SENSOR_GPIO, &gpio);
}

uint8_t SENSOR_Read(void)
{
    return (uint8_t)GPIO_ReadInputDataBit(APP_SENSOR_GPIO, APP_SENSOR_PIN);
}
