#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

#include "bsp_sys.h"

void delay_init(void);
void delay_us(uint32_t nus);
void delay_ms(uint32_t nms);

#endif
