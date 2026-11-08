#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include "app_config.h"
#include "stm32f10x.h"

void SENSOR_Init(void);
uint8_t SENSOR_Read(void);

#endif /* INPUT_H */
