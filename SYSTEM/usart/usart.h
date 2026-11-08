#ifndef DEBUG_USART_H
#define DEBUG_USART_H

#include <stdint.h>

/* USART1 PA9/PA10，作为 printf 调试输出。 */
void uart_init(uint32_t baudrate);

#endif /* DEBUG_USART_H */
