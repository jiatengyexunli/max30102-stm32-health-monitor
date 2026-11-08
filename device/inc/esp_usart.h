#ifndef ESP_USART_H
#define ESP_USART_H

#include <stdint.h>

/* USART2 PA2/PA3，ESP8266 AT 命令通道。 */
void uart2_Init(uint32_t baudrate);

#endif /* ESP_USART_H */
