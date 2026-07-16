#ifndef __BSP_4g_H
#define __BSP_4g_H

#include "stm32f4xx.h"
#include <stdint.h>
#include "bsp_usart.h"

#define USART_4G                USART6
#define USART_4G_BAUDRATE       115200
#define USART_4G_IRQn           USART6_IRQn

void bsp_USART6_4g_Init(uint32_t baudrate);
void USART_4G_SendString(const char *str);
void USART_4G_Init_Mutex(void);

#endif 
