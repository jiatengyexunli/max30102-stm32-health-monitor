#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include "app_config.h"

#define OLED_CMD   0U
#define OLED_DATA  1U

/*
 * SSD1306 128x64 软件 4 线 SPI 接口。
 * 引脚全部由 app_config.h 配置，避免再和串口/传感器抢占 GPIO。
 */
void OLED_Init(void);
void OLED_ClearBuffer(void);
void OLED_Clear(void);
void OLED_Refresh_Gram(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t dot);
void OLED_Fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, const uint8_t *text, uint8_t size);

#endif /* OLED_H */
