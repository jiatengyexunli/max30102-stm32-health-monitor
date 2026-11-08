#include "mpuiic.h"

#include "app_config.h"
#include "delay.h"

#define MPU_SCL_HIGH() (APP_MPU_I2C_GPIO->BSRR = APP_MPU_I2C_SCL_PIN)
#define MPU_SCL_LOW()  (APP_MPU_I2C_GPIO->BRR  = APP_MPU_I2C_SCL_PIN)
#define MPU_SDA_HIGH() (APP_MPU_I2C_GPIO->BSRR = APP_MPU_I2C_SDA_PIN)
#define MPU_SDA_LOW()  (APP_MPU_I2C_GPIO->BRR  = APP_MPU_I2C_SDA_PIN)
#define MPU_SDA_READ() ((APP_MPU_I2C_GPIO->IDR & APP_MPU_I2C_SDA_PIN) != 0U)

void MPU_IIC_Delay(void)
{
    delay_us(APP_I2C_DELAY_US);
}

void MPU_IIC_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    MPU_SCL_HIGH();
    MPU_SDA_HIGH();

    gpio.GPIO_Pin = APP_MPU_I2C_SCL_PIN | APP_MPU_I2C_SDA_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(APP_MPU_I2C_GPIO, &gpio);
}

void MPU_IIC_Start(void)
{
    MPU_SDA_HIGH();
    MPU_IIC_Delay();
    MPU_SCL_HIGH();
    MPU_IIC_Delay();
    MPU_SDA_LOW();
    MPU_IIC_Delay();
    MPU_SCL_LOW();
    MPU_IIC_Delay();
}

void MPU_IIC_Stop(void)
{
    MPU_SDA_LOW();
    MPU_IIC_Delay();
    MPU_SCL_HIGH();
    MPU_IIC_Delay();
    MPU_SDA_HIGH();
    MPU_IIC_Delay();
}

uint8_t MPU_IIC_Wait_Ack(void)
{
    uint16_t timeout;

    MPU_SDA_HIGH();
    MPU_IIC_Delay();
    MPU_SCL_HIGH();
    MPU_IIC_Delay();

    timeout = 0U;
    while (MPU_SDA_READ())
    {
        timeout++;
        if (timeout > 250U)
        {
            MPU_SCL_LOW();
            MPU_IIC_Stop();
            return 1U;
        }
        delay_us(1U);
    }
    MPU_SCL_LOW();
    MPU_IIC_Delay();
    return 0U;
}

void MPU_IIC_Ack(void)
{
    MPU_SCL_LOW();
    MPU_SDA_LOW();
    MPU_IIC_Delay();
    MPU_SCL_HIGH();
    MPU_IIC_Delay();
    MPU_SCL_LOW();
    MPU_SDA_HIGH();
}

void MPU_IIC_NAck(void)
{
    MPU_SCL_LOW();
    MPU_SDA_HIGH();
    MPU_IIC_Delay();
    MPU_SCL_HIGH();
    MPU_IIC_Delay();
    MPU_SCL_LOW();
}

void MPU_IIC_Send_Byte(uint8_t txd)
{
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++)
    {
        MPU_SCL_LOW();
        if ((txd & 0x80U) != 0U)
        {
            MPU_SDA_HIGH();
        }
        else
        {
            MPU_SDA_LOW();
        }
        txd <<= 1;
        MPU_IIC_Delay();
        MPU_SCL_HIGH();
        MPU_IIC_Delay();
    }
    MPU_SCL_LOW();
}

uint8_t MPU_IIC_Read_Byte(uint8_t ack)
{
    uint8_t bit;
    uint8_t value;

    value = 0U;
    MPU_SDA_HIGH();
    for (bit = 0U; bit < 8U; bit++)
    {
        MPU_SCL_LOW();
        MPU_IIC_Delay();
        MPU_SCL_HIGH();
        value <<= 1;
        if (MPU_SDA_READ())
        {
            value |= 1U;
        }
        MPU_IIC_Delay();
    }
    MPU_SCL_LOW();
    if (ack)
    {
        MPU_IIC_Ack();
    }
    else
    {
        MPU_IIC_NAck();
    }
    return value;
}
