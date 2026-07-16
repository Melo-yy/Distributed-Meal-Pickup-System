#include "bsp_scan.h"
volatile uint8_t uart_buf[UART_BUF_LEN] = {0};//用于保存串口数据
volatile uint8_t recv_len = 0; 					// 实际长度
volatile uint8_t recv_ok  = 0;					//接收完成标志

/**************************************************************************
Function: Scan device initialization
Input   : none
Output  : none
函数功能：扫描设备初始化
入口参数：无
返回  值：无
**************************************************************************/
void bsp_scan_init(void)
{
		//bsp_usart_init(SCAN_USART,9600);
}


/**************************************************************************
Function: Get decoded data from GM65 scanner
Input   : array - pointer to user buffer (≥16 bytes)
					len   - pointer to variable that will receive the actual length
Output  : 1  - new frame available and copied
					0  - no frame ready
函数功能：获取 GM65 扫描设备返回的条码数据
入口参数：array - 用户缓冲区首地址
					len   - 用于返回实际数据长度的指针
返回  值：1 表示成功获取并拷贝一帧数据, 0 表示暂无数据
**************************************************************************/
uint8_t bsp_scan_GetData(uint8_t *array, uint8_t *len)
{
    if (array == NULL || len == NULL) 
		{
        return 0;
    }
    
    if (!recv_ok) return 0;

    taskENTER_CRITICAL();  // FreeRTOS 临界区
    
    uint8_t copy_len = (recv_len > UART_BUF_LEN) ? UART_BUF_LEN : recv_len;  // 限制最大16字节
    for (uint8_t i = 0; i < copy_len; i++) 
	  {
        array[i] = uart_buf[i];
    }
    *len = copy_len;
    
    // 重置状态
    recv_ok = 0; 
    recv_len = 0;
		memset((void*)uart_buf, 0, UART_BUF_LEN); // 清空旧数据
    
    taskEXIT_CRITICAL();
    return 1;
}

// 串口中断
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(SCAN_USART, USART_IT_RXNE) != RESET)
    {
        uint8_t d = USART_ReceiveData(SCAN_USART);

        if (recv_ok == 0)               // 上一帧未取走则丢弃新字节
        {
            if (recv_len < UART_BUF_LEN - 1)   // 留 1 字节给 '\0' 或 '\n'
            {
                uart_buf[recv_len++] = d;

                if (d == 0x0D)          // 收到 CR，认为一帧结束
                {
                    uart_buf[recv_len] = '\0';   // 可追加 '\0' 方便字符串处理
                    recv_ok = 1;
                    /* 如需要，可在这里给出信号量/事件标志 */
									//printf("%s",uart_buf);
                }
            }
            else                        // 缓冲区满，强制结束
            {
                recv_len = 0;           // 丢弃整帧，重新开始
            }
        }
        // 显式清除中断标志
        USART_ClearITPendingBit(SCAN_USART, USART_IT_RXNE);
    }
}
