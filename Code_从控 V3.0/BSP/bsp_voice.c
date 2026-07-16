#include "bsp_voice.h"	


/**************************************************************************
Function: Voice module UART initialization (fixed 9600 bps)
Input   : none
Output  : none
函数功能：初始化语音模块所用的串口外设（VOICE_USART），波特率固定 9600
入口参数：无
返回  值：无
**************************************************************************/
//void bsp_voice_init(void)
//{
//		bsp_usart_init(VOICE_USART,9600);
//}


/**************************************************************************
Function: Voice module UART initialization (fixed 9600 bps)
Input   : none
Output  : none
函数功能：初始化语音模块所用的串口外设（VOICE_USART），波特率固定 9600
入口参数：无
返回  值：无
**************************************************************************/
void bsp_voice_send(uint8_t *Array, uint16_t Length)
{
		Serial_SendArray(USART1,Array,Length);
}



/**************************************************************************
Function: Send a 1-byte command packet to voice module
Input : msg_id - Message ID (command type)
				one_data - 1 byte data content
Output : none
函数功能：向语音模块发送单字节数据命令包，包含固定帧头帧尾格式
入口参数：msg_id - 消息号（命令类型）
					one_data - 单字节数据内容 （无需内容发送0）
返回 值：无
说明 ：数据包格式为：0xAA 0x55 [消息号] [数据] 0x55 0xAA
**************************************************************************/
void bsp_voice_send_byte(uint8_t msg_id, uint8_t one_data)
{
    uint8_t pkg[6] = {
        0xAA, 0x55,          // 帧头
        msg_id,              // 消息号（可改）
        one_data,            // 1 位数据（可改）
        0x55, 0xAA           // 帧尾
    };
    bsp_voice_send(pkg, 6);  // 整包发出
}

/**************************************************************************
Function: Send a 1-byte-command packet (no data) to voice module
Input : msg_id - Message ID (command type)
Output: none
函数功能：向语音模块发送“只有消息号”的命令包（无数据位）
入口参数：msg_id - 消息号（命令类型）
返回 值：无
说明  : 数据包格式：0xAA 0x55 [消息号] 0x55 0xAA
**************************************************************************/
void bsp_voice_send_cmd(uint8_t msg_id)
{
			uint8_t pkg[5] = {
			0xAA, 0x55,   // 帧头
			msg_id,       // 消息号
			0x55, 0xAA    // 帧尾
			};
			bsp_voice_send(pkg, 5);  // 整包发出
}
