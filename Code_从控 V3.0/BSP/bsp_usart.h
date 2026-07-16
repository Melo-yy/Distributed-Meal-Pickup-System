#ifndef __BSP_USART_H
#define __BSP_USART_H

#include "main.h"
#include "bsp_sys.h" 


#define			DEBUG_USART		USART1		//打印调试串口
#define			SCAN_USART		USART2		//二维码扫描模块串口
#define     VOICE_USART		USART3		//语音模块串口
#define			SHOW_USART		UART4			//串口屏显示串口


typedef struct {
    USART_TypeDef* USARTx;
    uint32_t USART_CLK;
    IRQn_Type USART_IRQn;
    GPIO_TypeDef* GPIO_PORT;
    uint32_t GPIO_CLK;
    uint16_t GPIO_TX_PIN;
    uint16_t GPIO_RX_PIN;
    uint8_t GPIO_AF;
    uint8_t GPIO_TX_PIN_SOURCE;
    uint8_t GPIO_RX_PIN_SOURCE;
} USART_ParamTypeDef;

static const USART_ParamTypeDef usart_param_table[] = {
    // USART1: PA9/PA10, AF7
    {USART1, RCC_APB2Periph_USART1, USART1_IRQn, GPIOA, RCC_AHB1Periph_GPIOA, GPIO_Pin_9,  GPIO_Pin_10,  GPIO_AF_USART1, GPIO_PinSource9,  GPIO_PinSource10},
    // USART2: PA2/PA3, AF7
    {USART2, RCC_APB1Periph_USART2, USART2_IRQn, GPIOA, RCC_AHB1Periph_GPIOA, GPIO_Pin_2,  GPIO_Pin_3,   GPIO_AF_USART2, GPIO_PinSource2,  GPIO_PinSource3},
    // USART3: PB10/PB11, AF7
    {USART3, RCC_APB1Periph_USART3, USART3_IRQn, GPIOB, RCC_AHB1Periph_GPIOB, GPIO_Pin_10, GPIO_Pin_11,  GPIO_AF_USART3, GPIO_PinSource10, GPIO_PinSource11},
    // UART4: PC10/PC11, AF8
    {UART4,  RCC_APB1Periph_UART4,  UART4_IRQn,  GPIOC, RCC_AHB1Periph_GPIOC, GPIO_Pin_10, GPIO_Pin_11,  GPIO_AF_UART4,  GPIO_PinSource10, GPIO_PinSource11},
};

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define LINE_BUFFER_SIZE 		128		//printf单行最大长度
	  	
extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		//接收状态标记	

// 通用串口初始化函数声明
void bsp_usart_init(USART_TypeDef* USARTx, uint32_t bound);
//DEBUG互斥锁初始化
void bsp_Debug_Mutex_init(void);  

// 串口相关函数声明
void Serial_SendByte(USART_TypeDef* USARTx, uint8_t Byte);
void Serial_SendArray(USART_TypeDef* USARTx, uint8_t *Array, uint16_t Length);
void Serial_SendString(USART_TypeDef* USARTx, char *String);
uint32_t Serial_Pow(uint32_t X, uint32_t Y);
void Serial_SendNumber(USART_TypeDef* USARTx, uint32_t Number, uint8_t Length);
void Serial_Printf(USART_TypeDef* USARTx, char *format, ...);

#endif


