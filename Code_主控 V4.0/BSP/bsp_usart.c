#include "bsp_usart.h"	

////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"					//FreeRTOS使用	  
#endif  


static char line_buffer[LINE_BUFFER_SIZE];
static uint16_t line_index = 0;
static SemaphoreHandle_t xLineMutex = NULL;


//使用 MicorLib,重定向fputc可正常使用printf
//int fputc(int ch, FILE *f)
//{
//		USART_SendData(USART1, (uint8_t)ch);
//		while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
//		return ch;
//}


int fputc(int ch, FILE *f)
{
#if SYSTEM_SUPPORT_OS
    if (xLineMutex != NULL) {
        xSemaphoreTake(xLineMutex, portMAX_DELAY);
    }
#endif

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
    
#if SYSTEM_SUPPORT_OS
    if (xLineMutex != NULL) {
        xSemaphoreGive(xLineMutex);
    }
#endif
    
    return ch;
}

/**************************************************************************
Function: 通用串口初始化（支持STM32F407的USART1/2/3和UART4）
Input   : USARTx——要初始化的串口外设（如USART1、USART2、USART3、UART4）
          bound——波特率（如115200、9600等）
Output  : none
函数功能：通用串口初始化
入口参数：USARTx：要初始化的串口外设
          bound：波特率
返回  值：无
**************************************************************************/
void bsp_usart_init(USART_TypeDef* USARTx, uint32_t bound)
{
    const USART_ParamTypeDef* param = NULL;
    for (int i = 0; i < sizeof(usart_param_table)/sizeof(usart_param_table[0]); i++) {
        if (usart_param_table[i].USARTx == USARTx) {
            param = &usart_param_table[i];
            break;
        }
    }
    if (!param) return; // 未找到参数，直接返回

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // 使能GPIO和USART时钟
    RCC_AHB1PeriphClockCmd(param->GPIO_CLK, ENABLE);
    if (USARTx == USART1)
        RCC_APB2PeriphClockCmd(param->USART_CLK, ENABLE);
    else
        RCC_APB1PeriphClockCmd(param->USART_CLK, ENABLE);

    // 配置复用功能
    GPIO_PinAFConfig(param->GPIO_PORT, param->GPIO_TX_PIN_SOURCE, param->GPIO_AF);
    GPIO_PinAFConfig(param->GPIO_PORT, param->GPIO_RX_PIN_SOURCE, param->GPIO_AF);

    // 配置GPIO
    GPIO_InitStructure.GPIO_Pin = param->GPIO_TX_PIN | param->GPIO_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(param->GPIO_PORT, &GPIO_InitStructure);

    // 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = param->USART_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 配置USART
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USARTx, &USART_InitStructure);

    USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
    USART_Cmd(USARTx, ENABLE);
		
}

void bsp_Debug_Mutex_init(void)
{
    if (xLineMutex == NULL) {
        xLineMutex = xSemaphoreCreateMutex();
    }
    line_index = 0;
}


/**************************************************************************
Function: 发送一个字节数据到指定串口
Input   : USARTx——目标串口外设指针
          Byte——要发送的数据
Output  : none
函数功能：向指定串口发送一个字节
入口参数：USARTx：目标串口外设
          Byte：要发送的数据
返回  值：无
**************************************************************************/
void Serial_SendByte(USART_TypeDef* USARTx, uint8_t Byte)
{
    USART_SendData(USARTx, Byte);
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
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
    for (uint16_t i = 0; i < Length; i++)
        Serial_SendByte(USARTx, Array[i]);
}

/**************************************************************************
Function: 发送字符串到指定串口
Input   : USARTx——目标串口外设指针
          String——字符串指针
Output  : none
函数功能：向指定串口发送字符串
入口参数：USARTx：目标串口外设
          String：字符串指针
返回  值：无
**************************************************************************/
void Serial_SendString(USART_TypeDef* USARTx, char *String)
{
    for (uint16_t i = 0; String[i] != '\0'; i++)
        Serial_SendByte(USARTx, String[i]);
}

/**************************************************************************
Function: 计算X的Y次幂
Input   : X——底数
          Y——指数
Output  : none
函数功能：返回X的Y次幂
入口参数：X：底数
          Y：指数
返回  值：X的Y次幂
**************************************************************************/
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) Result *= X;
    return Result;
}

/**************************************************************************
Function: 以指定长度发送无符号整数到串口
Input   : USARTx——目标串口外设指针
          Number——要发送的数字
          Length——发送的位数
Output  : none
函数功能：以定长方式向串口发送数字（高位补零）
入口参数：USARTx：目标串口外设
          Number：要发送的数字
          Length：发送的位数
返回  值：无
**************************************************************************/
void Serial_SendNumber(USART_TypeDef* USARTx, uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
        Serial_SendByte(USARTx, Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
}

/**************************************************************************
Function: 格式化输出到指定串口（类似printf）
Input   : USARTx——目标串口外设指针
          format——格式化字符串
          ...——可变参数
Output  : none
函数功能：格式化输出字符串到指定串口
入口参数：USARTx：目标串口外设
          format：格式化字符串
          ...：可变参数
返回  值：无
**************************************************************************/
void Serial_Printf(USART_TypeDef* USARTx, char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Serial_SendString(USARTx, String);
}





