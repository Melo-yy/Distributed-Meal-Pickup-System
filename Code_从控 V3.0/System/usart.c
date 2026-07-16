/*=======================usart.c======================*/
#include "usart.h"

// 定义互斥锁
static char line_buffer[LINE_BUFFER_SIZE];
static uint16_t line_index = 0;
static SemaphoreHandle_t xLineMutex = NULL;


void usart_mutex_init(void)
{
    if (xLineMutex == NULL) {
        xLineMutex = xSemaphoreCreateMutex();
        if (xLineMutex != NULL) {
            DBG_INFO("USART mutex initialized\r\n");
        } else {
            DBG_ERROR("Failed to create USART mutex!\r\n");
        }
    }
}

/**
 * 功能：串口初始化函数
 * 参数：None
 * 返回值：None
 */
void usart_init(unsigned int baud)
{
    GPIO_InitTypeDef GPIO_Init_Structure;                            //定义GPIO结构体
    USART_InitTypeDef USART_Init_Structure;                          //定义串口结构体
	NVIC_InitTypeDef  NVIC_Init_Structure;														//定义中断结构体
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
    RCC_APB2PeriphClockCmd(USART1_GPIO_CLK,  ENABLE);                 //开启GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,  ENABLE);            //开启APB2总线复用时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,  ENABLE);          //开启USART1时钟
    
    //配置PA9  TX
    GPIO_Init_Structure.GPIO_Mode  = GPIO_Mode_AF_PP;                //复用推挽
    GPIO_Init_Structure.GPIO_Pin   = USART1_TX_GPIO_PIN;
    GPIO_Init_Structure.GPIO_Speed = GPIO_Speed_50MHz;    
    GPIO_Init( USART1_GPIO_PORT, &GPIO_Init_Structure);
    
    //配置PA10 RX
    GPIO_Init_Structure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;                //复用推挽
    GPIO_Init_Structure.GPIO_Pin   = USART1_RX_GPIO_PIN;
    GPIO_Init( USART1_GPIO_PORT, &GPIO_Init_Structure);
    //串口相关配置
  	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);																					//串口中断配置
    USART_Init_Structure.USART_BaudRate = baud;                                          //波特率设置为115200
    USART_Init_Structure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;       //硬件流控制为无
    USART_Init_Structure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                       //模式设为收和发
    USART_Init_Structure.USART_Parity = USART_Parity_No;                                   //无校验位
    USART_Init_Structure.USART_StopBits = USART_StopBits_1;                                //一位停止位
    USART_Init_Structure.USART_WordLength = USART_WordLength_8b;                           //字长为8位   
    USART_Init(USART1, &USART_Init_Structure);                                             //初始化	
    USART_Cmd(USART1, ENABLE);                                                            //串口使能
		
		//中断结构体配置
	NVIC_Init_Structure.NVIC_IRQChannel 			=   USART1_IRQn;
	NVIC_Init_Structure.NVIC_IRQChannelCmd   	=   ENABLE;
	NVIC_Init_Structure.NVIC_IRQChannelPreemptionPriority  =  0;
	NVIC_Init_Structure.NVIC_IRQChannelSubPriority         =  1;
	NVIC_Init(&NVIC_Init_Structure);
   
}




void usart_init2(unsigned int baud)
{
    GPIO_InitTypeDef GPIO_Init_Structure;                            //定义GPIO结构体
    USART_InitTypeDef USART_Init_Structure;                          //定义串口结构体
		NVIC_InitTypeDef  NVIC_Init_Structure;														//定义中断结构体
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
    RCC_APB2PeriphClockCmd(USART2_GPIO_CLK,  ENABLE);                 //开启GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,  ENABLE);            //开启APB2总线复用时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,  ENABLE);          //开启USART1时钟
    
    //配置PA2 TX
    GPIO_Init_Structure.GPIO_Mode  = GPIO_Mode_AF_PP;                //复用推挽
    GPIO_Init_Structure.GPIO_Pin   = USART2_TX_GPIO_PIN;
    GPIO_Init_Structure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init( USART2_GPIO_PORT, &GPIO_Init_Structure);
    
    //配置PA3 RX
    GPIO_Init_Structure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;                //复用推挽
    GPIO_Init_Structure.GPIO_Pin   = USART2_RX_GPIO_PIN;
    GPIO_Init( USART2_GPIO_PORT, &GPIO_Init_Structure);
	
    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);	
    USART_Init_Structure.USART_BaudRate = 9600;                                          //波特率设置为115200
    USART_Init_Structure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;       //硬件流控制为无
    USART_Init_Structure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                       //模式设为收和发
    USART_Init_Structure.USART_Parity = USART_Parity_No;                                   //无校验位
    USART_Init_Structure.USART_StopBits = USART_StopBits_1;                                //一位停止位
    USART_Init_Structure.USART_WordLength = USART_WordLength_8b;                           //字长为8位  
    USART_Init(USART2, &USART_Init_Structure);  
    USART_Cmd(USART2, ENABLE);
		
		//中断结构体配置
	NVIC_Init_Structure.NVIC_IRQChannel 			=   USART2_IRQn;
	NVIC_Init_Structure.NVIC_IRQChannelCmd   	=   ENABLE;
	NVIC_Init_Structure.NVIC_IRQChannelPreemptionPriority  =  0;
	NVIC_Init_Structure.NVIC_IRQChannelSubPriority         =  3;
	NVIC_Init(&NVIC_Init_Structure);
}



void usart_init3(unsigned int baud)
{
    GPIO_InitTypeDef GPIO_Init_Structure;                            //定义GPIO结构体
    USART_InitTypeDef USART_Init_Structure;                          //定义串口结构体
  	NVIC_InitTypeDef  NVIC_Init_Structure;														//定义中断结构体
	  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
    RCC_APB2PeriphClockCmd(USART3_GPIO_CLK,  ENABLE);                 //开启GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,  ENABLE);            //开启APB2总线复用时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,  ENABLE);          //开启USART1时钟
    
    //配置PB10 TX
    GPIO_Init_Structure.GPIO_Mode  = GPIO_Mode_AF_PP;                //复用推挽
    GPIO_Init_Structure.GPIO_Pin   = USART3_TX_GPIO_PIN;
    GPIO_Init_Structure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init( USART3_GPIO_PORT, &GPIO_Init_Structure);
    
    //配置PB11 RX
    GPIO_Init_Structure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;                //复用推挽
    GPIO_Init_Structure.GPIO_Pin   = USART3_RX_GPIO_PIN;
    GPIO_Init( USART3_GPIO_PORT, &GPIO_Init_Structure);
	
    USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);	
    USART_Init_Structure.USART_BaudRate = baud;                                          //波特率设置
    USART_Init_Structure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;       //硬件流控制为无
    USART_Init_Structure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                       //模式设为收和发
    USART_Init_Structure.USART_Parity = USART_Parity_No;                                   //无校验位
    USART_Init_Structure.USART_StopBits = USART_StopBits_1;                                //一位停止位
    USART_Init_Structure.USART_WordLength = USART_WordLength_8b;                           //字长为8位   
    USART_Init(USART3, &USART_Init_Structure);   
    USART_Cmd(USART3, ENABLE);
		
		//中断结构体配置
	NVIC_Init_Structure.NVIC_IRQChannel 			=   USART3_IRQn;
	NVIC_Init_Structure.NVIC_IRQChannelCmd   	=   ENABLE;
	NVIC_Init_Structure.NVIC_IRQChannelPreemptionPriority  =  1;
	NVIC_Init_Structure.NVIC_IRQChannelSubPriority         =  3;
	NVIC_Init(&NVIC_Init_Structure);
}


//三个串口初始化函数
void Init_Usart(void)
{
	usart_init(9600);			// DEBUG & VOICE 公用
	usart_init2(9600);		// SCAN
	usart_init3(9600);		// SHOW
	
	usart_mutex_init();
}



/**
 * 功能：串口写字节函数
 * 参数1：USARTx ：串口号
 * 参数2：Data   ：需写入的字节
 * 返回值：None
 */
void USART_Send_Byte(USART_TypeDef* USARTx, uint16_t Data)
{
        USART_SendData(USARTx, Data);
				while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE)==RESET);
}
/**
 * 功能：串口写字符串函数
 * 参数1：USARTx ：串口号
 * 参数2：str    ：需写入的字符串
 * 返回值：None
 */
void USART_Send_String(USART_TypeDef* USARTx, char *str)
{
    // 如果互斥锁未创建，直接发送
    if (xLineMutex == NULL) {
				 uint16_t i=0;
				do
				{
						USART_Send_Byte(USARTx,  *(str+i));
						i++;
				}
				while(*(str + i) != '\0');
						
				while(USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET);
        return;
    }
		
    // 获取互斥锁
    if (xSemaphoreTake(xLineMutex, portMAX_DELAY) == pdTRUE) {
        uint16_t i=0;
				do
				{
						USART_Send_Byte(USARTx,  *(str+i));
						i++;
				}
				while(*(str + i) != '\0');
				while(USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET);
        xSemaphoreGive(xLineMutex);
    }
}

    

/**************************************************************************
Function: 发送数组到指定串口
Input   : USARTx——目标串口外设指针
          Array——数据数组指针
          Length——数组长度
Output  : none
函数功能：向指定串口连续发送多个字节
入口参数：USARTx：目标串口外设
          Array：数据数组指针
          Length：数组长度
返回  值：无
**************************************************************************/
void Serial_SendArray(USART_TypeDef* USARTx, uint8_t *Array, uint16_t Length)
{
    // 如果互斥锁未创建，直接发送
    if (xLineMutex == NULL) {
        for (uint16_t i = 0; i < Length; i++)
            USART_Send_Byte(USARTx, Array[i]);
        return;
    }
    
    // 获取互斥锁
    if (xSemaphoreTake(xLineMutex, portMAX_DELAY) == pdTRUE) {
        for (uint16_t i = 0; i < Length; i++)
            USART_Send_Byte(USARTx, Array[i]);
        xSemaphoreGive(xLineMutex);
    }
}
/**
 * 功能：重定向
 */
int fputc(int ch, FILE *f)
{
    if (xLineMutex != NULL) {
        xSemaphoreTake(xLineMutex, portMAX_DELAY);
    }

    // 添加到行缓冲区
    if (line_index < LINE_BUFFER_SIZE - 1) {
        line_buffer[line_index++] = (char)ch;
    }
    
    // 遇到换行或缓冲区满时发送整行
    if (ch == '\n' || line_index >= LINE_BUFFER_SIZE - 1) {
        // 发送整行
        for (uint16_t i = 0; i < line_index; i++) {
            USART_SendData(USART1, line_buffer[i]);
            while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        }
        line_index = 0;
    }
    
    if (xLineMutex != NULL) {
        xSemaphoreGive(xLineMutex);
    }    
		
    return ch;
}
/**
 * 功能：重定向
 */
int fgetc(FILE *f)
{
    while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==RESET);
    return (int)USART_ReceiveData(USART1);
}


////串口1中断
//void USART1_IRQHandler(void)
//{
//	volatile char temp;
//	if(USART_GetITStatus(USART1,USART_IT_RXNE)!= RESET)
//	{
//		temp = USART_ReceiveData(USART1);
//		USART_ClearITPendingBit(USART1,USART_IT_RXNE);	//清空标志位
//	}
//}


////串口2中断
//void USART2_IRQHandler(void)
//{
//	volatile char temp;
//	if(USART_GetITStatus(USART2,USART_IT_RXNE)!= RESET)
//	{
//		temp = USART_ReceiveData(USART2);
//		
//		USART_ClearITPendingBit(USART2,USART_IT_RXNE);	//清空标志位
//	}
//}

/*
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)
    {
       
    }
}*/


/*
void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2,USART_IT_RXNE) != RESET)
    {
       
    }
}*/



/*
void USART3_IRQHandler(void)
{
    char temp;
    if(USART_GetITStatus(USART3,USART_IT_RXNE) != RESET)
    {
        
    }
}
*/
