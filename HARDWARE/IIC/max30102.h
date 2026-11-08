#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>
#include "app_config.h"
#include "stm32f10x.h"

/* MAX30102 的 7 位地址为 0x57，写/读地址分别是 0xAE/0xAF。 */
#define MAX30102_ADDR_WRITE       0xAEU
#define MAX30102_ADDR_READ        0xAFU

/* 寄存器地址 */
#define REG_INTR_STATUS_1         0x00U
#define REG_INTR_STATUS_2         0x01U
#define REG_INTR_ENABLE_1         0x02U
#define REG_INTR_ENABLE_2         0x03U
#define REG_FIFO_WR_PTR           0x04U
#define REG_OVF_COUNTER           0x05U
#define REG_FIFO_RD_PTR           0x06U
#define REG_FIFO_DATA             0x07U
#define REG_FIFO_CONFIG           0x08U
#define REG_MODE_CONFIG           0x09U
#define REG_SPO2_CONFIG           0x0AU
#define REG_LED1_PA               0x0CU
#define REG_LED2_PA               0x0DU
#define REG_PILOT_PA              0x10U
#define REG_MULTI_LED_CTRL1       0x11U
#define REG_MULTI_LED_CTRL2       0x12U
#define REG_TEMP_INTR             0x1FU
#define REG_TEMP_FRAC             0x20U
#define REG_TEMP_CONFIG           0x21U
#define REG_PROX_INT_THRESH       0x30U
#define REG_REV_ID                0xFEU
#define REG_PART_ID               0xFFU

/* API 返回码 */
#define MAX30102_OK                 0
#define MAX30102_ERR_IO            -1
#define MAX30102_ERR_ID            -2
#define MAX30102_ERR_TIMEOUT       -3
#define MAX30102_ERR_CONFIG        -4

typedef struct
{
    uint32_t red;
    uint32_t ir;
} max30102_sample_t;

int max30102_init(void);
int max30102_reset(void);
int max30102_wait_for_sample(uint32_t timeout_ms);
int max30102_read_sample(max30102_sample_t *sample);

/* 底层寄存器接口保留为公开函数，便于串口诊断和单元测试。 */
uint8_t max30102_write_register(uint8_t reg, uint8_t value);
uint8_t max30102_read_register(uint8_t reg, uint8_t *value);

/* 兼容参考代码中的旧名称，新代码优先使用上面的小写 API。 */
uint8_t max30102_Bus_Write(uint8_t reg, uint8_t value);
uint8_t max30102_Bus_Read(uint8_t reg);

#endif /* MAX30102_H */
