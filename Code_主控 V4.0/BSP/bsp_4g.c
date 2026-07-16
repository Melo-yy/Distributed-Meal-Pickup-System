#include "bsp_4g.h"
#include "debug_config.h"

static SemaphoreHandle_t x4GMutex = NULL;

void USART_4G_Init_Mutex(void)
{
    if (x4GMutex == NULL) {
        x4GMutex = xSemaphoreCreateMutex();
        if (x4GMutex == NULL) {
            DBG_ERROR("[4G] 创建互斥锁失败！\n");
        } else {
            DBG_INFO("[4G] 互斥锁创建成功\n");
        }
    }
}

void bsp_USART6_4g_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

    // TX: PC6
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // RX: PC7
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART_4G, &USART_InitStructure);

    USART_ITConfig(USART_4G, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART_4G, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART_4G_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
		
		//初始化互斥锁
		USART_4G_Init_Mutex();
}

void USART_4G_SendByte(uint8_t data)
{
    while (USART_GetFlagStatus(USART_4G, USART_FLAG_TXE) == RESET);
    USART_SendData(USART_4G, data);
}

void USART_4G_SendBuffer(uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        USART_4G_SendByte(buf[i]);
    }
}

//void USART_4G_SendString(const char *str)
//{
//    while (*str)
//    {
//        USART_4G_SendByte(*str++);
//    }
//}

// 带互斥锁保护的发送函数
void USART_4G_SendString(const char *str)
{
    if (str == NULL) return;
    
    // 尝试获取互斥锁（带超时，避免永久阻塞）
    if (x4GMutex != NULL) {
        if (xSemaphoreTake(x4GMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // 成功获取锁，发送数据
            while (*str) {
                USART_4G_SendByte(*str++);
            }
            
            // 释放锁
            xSemaphoreGive(x4GMutex);
        } else {
            // 获取锁超时（可选：记录错误或重试）
            DBG_ERROR("[4G] 发送超时，数据可能丢失: %s\n", str);
            // 紧急情况：可不带锁发送（风险自担）
            // USART_4G_SendString(str);
        }
    } else {
        // 互斥锁未初始化，直接发送（不安全）
        USART_4G_SendString(str);
    }
}

