#include "max30102.h"

#include "delay.h"
#include "myiic.h"
#include "timebase.h"

#define MAX30102_INTERRUPT_ACTIVE() \
    ((GPIO_ReadInputDataBit(APP_MAX_INT_GPIO, APP_MAX_INT_PIN) == Bit_RESET) ? 1U : 0U)

uint8_t max30102_write_register(uint8_t reg, uint8_t value)
{
    return IIC_Write_One_Byte(MAX30102_ADDR_WRITE, reg, value);
}

uint8_t max30102_read_register(uint8_t reg, uint8_t *value)
{
    return IIC_Read_One_Byte(MAX30102_ADDR_WRITE, reg, value);
}

/* 兼容旧代码的名称；返回值 1 表示成功，与旧工程保持一致。 */
uint8_t max30102_Bus_Write(uint8_t reg, uint8_t value)
{
    return (max30102_write_register(reg, value) == 0U) ? 1U : 0U;
}

uint8_t max30102_Bus_Read(uint8_t reg)
{
    uint8_t value;

    value = 0xFFU;
    if (max30102_read_register(reg, &value) != 0U)
    {
        return 0U;
    }
    return value;
}

int max30102_reset(void)
{
    GPIO_InitTypeDef gpio;
    uint8_t mode;
    uint8_t retry;
    uint32_t start;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Pin = APP_MAX_INT_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(APP_MAX_INT_GPIO, &gpio);

    IIC_Init();
    if (max30102_write_register(REG_MODE_CONFIG, 0x40U) != 0U)
    {
        return MAX30102_ERR_IO;
    }

    /*
     * RESET 位由器件自动清零。轮询期间被复位打断而产生的单次 NACK
     * 不应立即判死，因此给 200 ms 重试窗口。
     */
    start = Timebase_GetMillis();
    retry = 0U;
    while (!Timebase_Elapsed(start, 200U))
    {
        if (max30102_read_register(REG_MODE_CONFIG, &mode) == 0U &&
            (mode & 0x40U) == 0U)
        {
            delay_ms(10U);
            return MAX30102_OK;
        }
        retry++;
        delay_ms(2U);
    }
    return MAX30102_ERR_TIMEOUT;
}

int max30102_init(void)
{
    uint8_t part_id;
    uint8_t revision;
    uint8_t dummy;

    if (max30102_reset() != MAX30102_OK)
    {
        return MAX30102_ERR_TIMEOUT;
    }

    if (max30102_read_register(REG_PART_ID, &part_id) != 0U)
    {
        return MAX30102_ERR_IO;
    }
    if (part_id != 0x15U)
    {
        return MAX30102_ERR_ID;
    }
    (void)max30102_read_register(REG_REV_ID, &revision);

    /* 读取状态寄存器清掉上电/复位遗留的中断。 */
    (void)max30102_read_register(REG_INTR_STATUS_1, &dummy);
    (void)max30102_read_register(REG_INTR_STATUS_2, &dummy);

    /* 使能 PPG_RDY 和 FIFO_FULL 中断；当前通过 INT 引脚轮询 PPG_RDY。 */
    if (max30102_write_register(REG_INTR_ENABLE_1, 0xC0U) != 0U)
    {
        return MAX30102_ERR_CONFIG;
    }
    if (max30102_write_register(REG_INTR_ENABLE_2, 0x00U) != 0U)
    {
        return MAX30102_ERR_CONFIG;
    }

    /* 清空 FIFO 指针和溢出计数。 */
    if (max30102_write_register(REG_FIFO_WR_PTR, 0x00U) != 0U ||
        max30102_write_register(REG_OVF_COUNTER, 0x00U) != 0U ||
        max30102_write_register(REG_FIFO_RD_PTR, 0x00U) != 0U)
    {
        return MAX30102_ERR_CONFIG;
    }

    /*
     * FIFO_CONFIG=0x3F：
     *   sample average=2、rollover enable、almost full remaining=15。
     * MODE_CONFIG=0x03：SpO2 模式。
     * SPO2_CONFIG=0x27：4096 nA、100 Hz、411 us/18-bit。
     */
    if (max30102_write_register(REG_FIFO_CONFIG, 0x3FU) != 0U ||
        max30102_write_register(REG_MODE_CONFIG, 0x03U) != 0U ||
        max30102_write_register(REG_SPO2_CONFIG, 0x27U) != 0U)
    {
        return MAX30102_ERR_CONFIG;
    }

    if (max30102_write_register(REG_LED1_PA, APP_MAX_LED_CURRENT_REG) != 0U ||
        max30102_write_register(REG_LED2_PA, APP_MAX_LED_CURRENT_REG) != 0U ||
        max30102_write_register(REG_PILOT_PA, 0x3FU) != 0U)
    {
        return MAX30102_ERR_CONFIG;
    }

    delay_ms(10U);
    return MAX30102_OK;
}

int max30102_wait_for_sample(uint32_t timeout_ms)
{
    uint32_t start;

    start = Timebase_GetMillis();
    while (!MAX30102_INTERRUPT_ACTIVE())
    {
        if (Timebase_Elapsed(start, timeout_ms))
        {
            return MAX30102_ERR_TIMEOUT;
        }
        delay_us(200U);
    }
    return MAX30102_OK;
}

int max30102_read_sample(max30102_sample_t *sample)
{
    uint8_t bytes[6];

    if (sample == 0)
    {
        return MAX30102_ERR_IO;
    }

    /* 先读状态寄存器，清除 PPG_RDY/FIFO_FULL。 */
    if (IIC_ReadBytes(MAX30102_ADDR_WRITE, REG_INTR_STATUS_1, bytes, 1U) != 0U)
    {
        return MAX30102_ERR_IO;
    }
    if (IIC_ReadBytes(MAX30102_ADDR_WRITE, REG_INTR_STATUS_2, bytes, 1U) != 0U)
    {
        return MAX30102_ERR_IO;
    }

    /*
     * FIFO 每个样本 6 字节：RED[23:0] + IR[23:0]。
     * ADC 实际为 18 bit，因此低 2 位保留，高 2 位清零。
     */
    if (IIC_ReadBytes(MAX30102_ADDR_WRITE, REG_FIFO_DATA, bytes, 6U) != 0U)
    {
        return MAX30102_ERR_IO;
    }

    sample->red = (((uint32_t)bytes[0] & 0x03U) << 16) |
                  ((uint32_t)bytes[1] << 8) |
                  (uint32_t)bytes[2];
    sample->ir = (((uint32_t)bytes[3] & 0x03U) << 16) |
                 ((uint32_t)bytes[4] << 8) |
                 (uint32_t)bytes[5];
    return MAX30102_OK;
}
