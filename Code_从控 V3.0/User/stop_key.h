#ifndef __STOP_KEY_H__
#define __STOP_KEY_H__
#include "stm32f10x.h"                  // Device header


typedef struct{
	u8 flag1;					//该电机按键1的状态
	u8 flag2;					//该电机按键2的状态
	u16 cnt1;					//该电机按键1的计时
	u16 cnt2;					//该电机按键2的计时
	u8 bit1;				//该电机按键1运行时的状态
	u8 bit2;				//该电机按键2运行时的状态
}KEY_STATE;

extern KEY_STATE Step1_Key,Step2_Key,Step3_Key,Step4_Key;

#define Step_Key_TIM_CLK					RCC_APB1Periph_TIM6
#define Step_Key_TIM							TIM6		
#define Step_Key_TIM_Period				100-1	
#define Step_Key_TIM_Prescaler		7200-1
#define Step_Key_TIM_IRQn		      TIM6_IRQn

#define Step_Key_CLK					RCC_APB2Periph_GPIOG	
#define Step_Key_port					GPIOG

#define Step1_Key_1_Pin				GPIO_Pin_0			
#define Step1_Key_2_Pin				GPIO_Pin_1			
#define Step2_Key_1_Pin				GPIO_Pin_2			
#define Step2_Key_2_Pin				GPIO_Pin_3			
#define Step3_Key_1_Pin				GPIO_Pin_4			
#define Step3_Key_2_Pin				GPIO_Pin_5			
#define Step4_Key_1_Pin				GPIO_Pin_6			
#define Step4_Key_2_Pin				GPIO_Pin_7


void Stop_Key_Init(void);

/********************************
使用方法:
	直接使用全局结构体变量,判断对应按键是否等于3,等于表示按下
	
	eg.if(Mykey.key_1 == 3)
		{
			//如果按键1按下执行这里
		}
********************************/
#endif
