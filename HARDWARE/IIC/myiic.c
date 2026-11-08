#include "myiic.h"
#include "app_config.h"
#include "delay.h"

#define IIC_SCL_HIGH() (APP_MAX_I2C_GPIO->BSRR = APP_MAX_I2C_SCL_PIN)
#define IIC_SCL_LOW()  (APP_MAX_I2C_GPIO->BRR  = APP_MAX_I2C_SCL_PIN)
#define IIC_SDA_HIGH() (APP_MAX_I2C_GPIO->BSRR = APP_MAX_I2C_SDA_PIN)
#define IIC_SDA_LOW()  (APP_MAX_I2C_GPIO->BRR  = APP_MAX_I2C_SDA_PIN)
#define IIC_SDA_READ() ((APP_MAX_I2C_GPIO->IDR & APP_MAX_I2C_SDA_PIN) != 0U)

static void iic_half_cycle_delay(void)
{
    delay_us(APP_I2C_DELAY_US);
}

void IIC_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /*
     * 先把输出锁存器置 1，再把引脚切换成开漏输出，避免初始化瞬间无谓拉低。
     * 50 MHz 配置只影响边沿速度，实际 SCL 频率由 iic_half_cycle_delay 控制。
     */
    IIC_SCL_HIGH();
    IIC_SDA_HIGH();
    gpio.GPIO_Pin = APP_MAX_I2C_SCL_PIN | APP_MAX_I2C_SDA_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(APP_MAX_I2C_GPIO, &gpio);
}

void IIC_Start(void)
{
    /* 空闲时 SCL/SDA 均为高；SCL 高电平期间 SDA 下降沿为 START。 */
    IIC_SDA_HIGH();
    iic_half_cycle_delay();
    IIC_SCL_HIGH();
    iic_half_cycle_delay();
    IIC_SDA_LOW();
    iic_half_cycle_delay();
    IIC_SCL_LOW();
    iic_half_cycle_delay();
}

void IIC_Stop(void)
{
    /* SCL 高电平期间 SDA 上升沿为 STOP。 */
    IIC_SDA_LOW();
    iic_half_cycle_delay();
    IIC_SCL_HIGH();
    iic_half_cycle_delay();
    IIC_SDA_HIGH();
    iic_half_cycle_delay();
}

uint8_t IIC_Wait_Ack(void)
{
    uint16_t timeout;

    IIC_SDA_HIGH();
    iic_half_cycle_delay();
    IIC_SCL_HIGH();
    iic_half_cycle_delay();

    timeout = 0U;
    while (IIC_SDA_READ())
    {
        timeout++;
        if (timeout > 250U)
        {
            IIC_SCL_LOW();
            IIC_Stop();
            return 1U;
        }
        delay_us(1U);
    }

    IIC_SCL_LOW();
    iic_half_cycle_delay();
    return 0U;
}

void IIC_Ack(void)
{
    IIC_SCL_LOW();
    IIC_SDA_LOW();
    iic_half_cycle_delay();
    IIC_SCL_HIGH();
    iic_half_cycle_delay();
    IIC_SCL_LOW();
    IIC_SDA_HIGH();
}

void IIC_NAck(void)
{
    IIC_SCL_LOW();
    IIC_SDA_HIGH();
    iic_half_cycle_delay();
    IIC_SCL_HIGH();
    iic_half_cycle_delay();
    IIC_SCL_LOW();
}

void IIC_Send_Byte(uint8_t txd)
{
    uint8_t bit;

    for (bit = 0U; bit < 8U; bit++)
    {
        IIC_SCL_LOW();
        if ((txd & 0x80U) != 0U)
        {
            IIC_SDA_HIGH();
        }
        else
        {
            IIC_SDA_LOW();
        }
        txd <<= 1;
        iic_half_cycle_delay();
        IIC_SCL_HIGH();
        iic_half_cycle_delay();
    }
    IIC_SCL_LOW();
}

uint8_t IIC_Read_Byte(uint8_t ack)
{
    uint8_t bit;
    uint8_t value;

    value = 0U;
    IIC_SDA_HIGH();
    for (bit = 0U; bit < 8U; bit++)
    {
        IIC_SCL_LOW();
        iic_half_cycle_delay();
        IIC_SCL_HIGH();
        value <<= 1;
        if (IIC_SDA_READ())
        {
            value |= 1U;
        }
        iic_half_cycle_delay();
    }
    IIC_SCL_LOW();

    if (ack)
    {
        IIC_Ack();
    }
    else
    {
        IIC_NAck();
    }
    return value;
}

uint8_t IIC_Write_One_Byte(uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    IIC_Start();
    IIC_Send_Byte(dev_addr);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }
    IIC_Send_Byte(reg);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }
    IIC_Send_Byte(data);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }
    IIC_Stop();
    delay_us(100U);
    return 0U;
}

uint8_t IIC_Read_One_Byte(uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
    if (data == 0)
    {
        return 1U;
    }

    IIC_Start();
    IIC_Send_Byte(dev_addr & 0xFEU);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }
    IIC_Send_Byte(reg);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }

    IIC_Start();
    IIC_Send_Byte(dev_addr | 0x01U);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }
    *data = IIC_Read_Byte(0U);
    IIC_Stop();
    return 0U;
}

uint8_t IIC_WriteBytes(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t length)
{
    uint16_t i;

    if ((data == 0) || (length == 0U))
    {
        return 1U;
    }

    IIC_Start();
    IIC_Send_Byte(dev_addr & 0xFEU);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }
    IIC_Send_Byte(reg);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }

    for (i = 0U; i < length; i++)
    {
        IIC_Send_Byte(data[i]);
        if (IIC_Wait_Ack())
        {
            IIC_Stop();
            return 1U;
        }
    }
    IIC_Stop();
    return 0U;
}

uint8_t IIC_ReadBytes(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t length)
{
    uint16_t i;

    if ((data == 0) || (length == 0U))
    {
        return 1U;
    }

    IIC_Start();
    IIC_Send_Byte(dev_addr & 0xFEU);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }
    IIC_Send_Byte(reg);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }

    IIC_Start();
    IIC_Send_Byte(dev_addr | 0x01U);
    if (IIC_Wait_Ack())
    {
        IIC_Stop();
        return 1U;
    }

    for (i = 0U; i < length; i++)
    {
        data[i] = IIC_Read_Byte((uint8_t)((i + 1U) < length));
    }
    IIC_Stop();
    return 0U;
}
