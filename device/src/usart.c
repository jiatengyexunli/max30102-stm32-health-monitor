#include "esp_usart.h"
#include "esp8266.h"

#include "app_config.h"
#include "stm32f10x.h"

void uart2_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = baudrate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &usart);

    nvic.NVIC_IRQChannel = USART2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 2;
    nvic.NVIC_IRQChannelSubPriority = 1;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

/*
 * ESP8266 返回的数据按“字节中断 + 总线空闲中断”收帧。
 * complete 表示一段总线空闲已经发生；AT 驱动还会等待一个短暂静默期，
 * 以覆盖 “+CWJAP:.../OK” 这类分多行返回的响应。
 */
void USART2_IRQHandler(void)
{
    uint8_t byte;
    volatile uint32_t clear;

    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        byte = (uint8_t)USART_ReceiveData(USART2);
        if (g_esp8266_rx.length < (APP_ESP8266_RX_BUFFER_SIZE - 1U))
        {
            g_esp8266_rx.buffer[g_esp8266_rx.length++] = (char)byte;
        }
        else
        {
            g_esp8266_rx.overflow = 1U;
        }
    }

    if (USART_GetFlagStatus(USART2, USART_FLAG_IDLE) != RESET)
    {
        clear = USART2->SR;
        clear = USART2->DR;
        (void)clear;
        g_esp8266_rx.buffer[g_esp8266_rx.length] = '\0';
        g_esp8266_rx.complete = 1U;
    }
}
