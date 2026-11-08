#include "sys.h"

void NVIC_Configuration(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

void JTAG_Set(uint8_t mode)
{
    uint32_t value;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    value = ((uint32_t)mode & 0x07U) << 24;
    AFIO->MAPR = (AFIO->MAPR & ~(0x07U << 24)) | value;
}
