#ifndef __BSP_USART_H
#define __BSP_USART_H

#include "stdio.h"
#include "stdarg.h"
#include "stm32f4xx_conf.h"
#include "bsp_sys.h"

#define DEBUG_USART     USART1

void bsp_usart_init(USART_TypeDef* USARTx, uint32_t bound);
void Serial_SendByte(USART_TypeDef* USARTx, uint8_t Byte);
void Serial_SendString(USART_TypeDef* USARTx, char *String);
void Serial_Printf(USART_TypeDef* USARTx, char *format, ...);

#endif
