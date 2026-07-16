#ifndef __BSP_SCAN_H
#define __BSP_SCAN_H

#include "stm32f4xx.h" 
#include "bsp_usart.h"


#define UART_BUF_LEN  24        				// 根据最大帧长调整


void bsp_scan_init(void);
uint8_t bsp_scan_GetData(uint8_t *array, uint8_t *len);


#endif
