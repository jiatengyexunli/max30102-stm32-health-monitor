#ifndef MPUIIC_H
#define MPUIIC_H

#include "stm32f10x.h"

/*
 * MPU6050 专用软件 I2C。
 * PB10=SCL，PB11=SDA，使用开漏输出和外部上拉。
 */
void MPU_IIC_Delay(void);
void MPU_IIC_Init(void);
void MPU_IIC_Start(void);
void MPU_IIC_Stop(void);
void MPU_IIC_Send_Byte(uint8_t txd);
uint8_t MPU_IIC_Read_Byte(uint8_t ack);
uint8_t MPU_IIC_Wait_Ack(void);
void MPU_IIC_Ack(void);
void MPU_IIC_NAck(void);

#endif /* MPUIIC_H */
