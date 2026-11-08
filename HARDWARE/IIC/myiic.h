#ifndef MYIIC_H
#define MYIIC_H

#include "stm32f10x.h"

/*
 * 通用软件 I2C，当前供 MAX30102 使用。
 *
 * 引脚采用“开漏输出 + 外部上拉”：
 * - 写 1 实际是释放总线，由上拉电阻拉高；
 * - 写 0 时 MOSFET 拉低总线；
 * - 读取 SDA 直接读 IDR。
 *
 * 因此 SCL/SDA 外部必须有上拉电阻。MAX30102 模块一般已经集成，
 * 若自行连接裸芯片，请确认上拉阻值和电源电压。
 */
void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
uint8_t IIC_Wait_Ack(void);                    /* 0=ACK，1=NACK/超时 */
void IIC_Ack(void);
void IIC_NAck(void);
void IIC_Send_Byte(uint8_t txd);
uint8_t IIC_Read_Byte(uint8_t ack);            /* ack=1 读完发送 ACK */

/* 返回值：0=成功，1=设备 NACK/总线错误。 */
uint8_t IIC_Write_One_Byte(uint8_t dev_addr, uint8_t reg, uint8_t data);
uint8_t IIC_Read_One_Byte(uint8_t dev_addr, uint8_t reg, uint8_t *data);
uint8_t IIC_WriteBytes(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t length);
uint8_t IIC_ReadBytes(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t length);

#endif /* MYIIC_H */
