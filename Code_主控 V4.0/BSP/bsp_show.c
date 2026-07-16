#include "bsp_show.h"

/**************************************************************************
Function: Initialize the “show” USART interface (fixed 9600 baud)
Input   : none
Output  : none
函数功能：显示串口初始化（波特率固定 9600）
入口参数：无
返回  值：无
**************************************************************************/
void bsp_show_init(void)
{
    bsp_usart_init(SHOW_USART, 9600);
}


/**************************************************************************
Function: Send a string to the “show” USART
Input   : String – pointer to the '\0'-terminated ASCII string
Output  : none
函数功能：通过显示串口发送一条字符串
入口参数：String – 待发送的 ASCII 字符串指针
返回  值：无
**************************************************************************/
void bsp_show_switch(const char* fmt, ...)
{
    char buffer[100];
    va_list ap;
        
	  // 处理可变参数
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap); 
    va_end(ap);
	
    Serial_SendString(SHOW_USART, buffer);
}

//切换界面
// page x 
//x为页面编号
