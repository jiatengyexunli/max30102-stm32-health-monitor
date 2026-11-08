#ifndef SYS_H
#define SYS_H

#include "stm32f10x.h"

/* 中断优先级分组只初始化一次，所有外设共用 2 位抢占 + 2 位子优先级。 */
void NVIC_Configuration(void);

/*
 * 关闭 JTAG 并保留 SWD，释放 PA15/PB3/PB4 中本项目需要使用的引脚。
 * mode 直接写入 AFIO->MAPR 的 SWJ_CFG[2:0]。
 */
void JTAG_Set(uint8_t mode);

#define JTAG_SWD_DISABLE   0x02U
#define SWD_ENABLE         0x01U
#define JTAG_SWD_ENABLE    0x00U

#endif /* SYS_H */
