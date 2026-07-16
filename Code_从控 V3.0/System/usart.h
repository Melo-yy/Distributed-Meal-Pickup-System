/*========================usart.h=====================*/

#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "debug_config.h"

//串口1
#define USART1_GPIO_PORT      GPIOA
#define USART1_GPIO_CLK       RCC_APB2Periph_GPIOA
#define USART1_TX_GPIO_PIN    GPIO_Pin_9
#define USART1_RX_GPIO_PIN    GPIO_Pin_10

//串口2
#define SCAN_USART      			USART2
#define USART2_GPIO_PORT      GPIOA
#define USART2_GPIO_CLK       RCC_APB2Periph_GPIOA
#define USART2_TX_GPIO_PIN    GPIO_Pin_2
#define USART2_RX_GPIO_PIN    GPIO_Pin_3

//串口3
#define SHOW_USART      			USART3
#define USART3_GPIO_PORT      GPIOB
#define USART3_GPIO_CLK       RCC_APB2Periph_GPIOB
#define USART3_TX_GPIO_PIN    GPIO_Pin_10
#define USART3_RX_GPIO_PIN    GPIO_Pin_11 

#define LINE_BUFFER_SIZE 		128		//printf单行最大长度



void usart_init(unsigned int baud);
void usart_init2(unsigned int baud);
void usart_init3(unsigned int baud);
void Init_Usart(void);
void USART_Send_Byte(USART_TypeDef* USARTx, uint16_t Data);
void USART_Send_String(USART_TypeDef* USARTx, char *str);
void Serial_SendArray(USART_TypeDef* USARTx, uint8_t *Array, uint16_t Length);

#endif  /* __USART_H */

