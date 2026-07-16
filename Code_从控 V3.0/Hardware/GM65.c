#include "stm32f10x.h"
#include <stdio.h>

u8 recv_ok = 0;       //接收完成标志
u8 uart_buf[32]={0};  //用于保存串口数据
u8 uart_cnt=0;        //用于定位串口数据的位置

//获取GM65返回的数据
//参数array：需要传入数据的数组
//传入成功时返回1
unsigned char GM65_GetData(unsigned char *array)
{
	unsigned char i;
	if(recv_ok==1){
		for(i=0;i<16;i++){
			array[i]=uart_buf[i];
		}
		recv_ok=0;
		return 1;
	}
	return 0;
}


//使用串口3中断处理函数
//void USART3_IRQHandler(void)      //串口3中断服务程序
//{
//	uint8_t d;

//	//检测标志位
//	if(USART_GetITStatus(USART3, USART_IT_RXNE)==SET)
//	{
//		//接收数据
//		d = USART_ReceiveData(USART3);
//	    //将接收到的数据依次保存到数组里
//		uart_buf[uart_cnt++] = d;  
//		//GM65模块发完一组数据后会自动发送一个回车符，所以通过检测是否接受到回车来判断数据是否接收完成
//		if(d == 0x0D) 
//		{
//			recv_ok = 1;  //接收完成
//			uart_cnt = 0; //最后清零，重新计数
//		}
//			//将接收到的数据，通过串口1返发给PC
//		USART_SendData(USART1, d);
//	    while( USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET); //等待发送完成
//		USART_ClearFlag(USART1,USART_FLAG_TXE);         //清空标志位
//		USART_ClearITPendingBit(USART3,USART_IT_RXNE);	//清空标志位
//	}	
//} 



/*
//串口3
#define USART3_GPIO_PORT      GPIOB
#define USART3_GPIO_CLK       RCC_APB2Periph_GPIOB
#define USART3_TX_GPIO_PIN    GPIO_Pin_10
#define USART3_RX_GPIO_PIN    GPIO_Pin_11

串口3设置函数
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
    USART_Init_Structure.USART_BaudRate = baud;                                         	//波特率设置
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
*/
