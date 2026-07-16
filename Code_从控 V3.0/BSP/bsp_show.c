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
    usart_init3(9600);
}


/**************************************************************************
Function: fast_uint_to_str
Input   : buf, val
Output  : char* - pointer to the null terminator
函数功能：快速将无符号整数转换为字符串。采用逆序提取数字后就地反转的算法
入口参数：buf-存储结果的缓冲区指针，
					val-待转换的 32 位无符号整数
返回  值：指向转换后字符串末尾结束符（'\0'）的指针
**************************************************************************/
static char* fast_uint_to_str(char* buf, uint32_t val)
{
    char* start = buf;

    if (val == 0) {
        *buf++ = '0';
        *buf = '\0';
        return buf;  // 返回 \0 后
    }

    // 从低位到高位提取数字，逆序写入
    char* p = buf;
    while (val > 0) {
        *p++ = '0' + (val % 10);
        val /= 10;
    }
    *p = '\0';  // 先加结束符

    // 逆序修正（经典就地反转）
    char* left = buf;
    char* right = p - 1;
    while (left < right) {
        char temp = *left;
        *left++ = *right;
        *right-- = temp;
    }

    return p;  // 返回 \0 后的位置 ✓
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
    char buffer[32] = {0};  // 清零保险

    va_list ap;
    va_start(ap, fmt);

    if (strncmp(fmt, "page ", 5) == 0) {
        int page_num = va_arg(ap, int);

        // 直接用 snprintf 只转数字部分（Keil 下 %d 很安全，不会吃大栈）
        snprintf(buffer, sizeof(buffer), "page %d", page_num);

        // 手动追加结尾三个 0xFF（关键！）
        size_t len = strlen(buffer);
        if (len + 3 < sizeof(buffer)) {
            buffer[len++] = (char)0xFF;
            buffer[len++] = (char)0xFF;
            buffer[len++] = (char)0xFF;
            buffer[len] = '\0';  // 可选，Send_String 不依赖 \0
        }
    }
    else if (strncmp(fmt, "n0.val=", 7) == 0) {
        int value = va_arg(ap, int);

        snprintf(buffer, sizeof(buffer), "n0.val=%d", value);

        size_t len = strlen(buffer);
        if (len + 3 < sizeof(buffer)) {
            buffer[len++] = (char)0xFF;
            buffer[len++] = (char)0xFF;
            buffer[len++] = (char)0xFF;
            buffer[len] = '\0';
        }
    }

    va_end(ap);

    // 调试必加：打印实际发送内容（16进制看 0xFF）
    DBG_INFO("SEND DISPLAY: ");
    for (int i = 0; buffer[i]; i++) {
        if ((unsigned char)buffer[i] >= 0x20 && (unsigned char)buffer[i] <= 0x7E) {
            putchar(buffer[i]);
        } else {
            DBG_INFO("\\x%02X", (unsigned char)buffer[i]);
        }
    }
    DBG_INFO("\n");

    USART_Send_String(SHOW_USART, buffer);
}

