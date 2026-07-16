#include "stm32f10x.h"                  // Device header
#include "Step.h"
#include "./delay/delay.h"

/**
  * 函    数：步进电机与微动开关初始化
  * 参    数：无
  * 返 回 值：无
  *注     意：共地，参数默认(158-1,72-1)
  */
void Step_Init(void)			
{
    //初始化步进电机定时器
	RCC_APB1PeriphClockCmd(Step_TIM_CLK, ENABLE);
    //初始化步进电机的时钟
	RCC_APB2PeriphClockCmd(Step_PUL_CLK | Step_DIR_CLK, ENABLE);
	
	//脉冲引脚
    GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = Step1_Pul_Pin | Step2_Pul_Pin ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_PUL_port, &GPIO_InitStructure);
	
	//方向引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = Step1_DIR_Pin | Step2_DIR_Pin ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_DIR_port, &GPIO_InitStructure);
	
    //选择时基单元时钟
	TIM_InternalClockConfig(TIM2);
	
    //配置时基单元
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = Step_TIM_Period;
	TIM_TimeBaseInitStructure.TIM_Prescaler = Step_TIM_Prescaler;		
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(Step_TIM, &TIM_TimeBaseInitStructure);
	
    //输出比较初始化
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;		//CCR
	TIM_OC1Init(Step_TIM, &TIM_OCInitStructure);
	TIM_OC2Init(Step_TIM, &TIM_OCInitStructure);
	
    //启动定时器
	TIM_Cmd(Step_TIM, ENABLE);
	
    //微动开关
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
  * 函    数：开启指定电机
  * 参    数：Step_num：Step1, Step2, Step3, Step4
  * 参    数：Step_DIR：Step_DIR_Forward, Step_DIR_Counter
  * 返 回 值：无
  */
void Step_ON(Step Step_num, BitAction Step_DIR)
{
	switch (Step_num) 
	{
		case Step1:
            GPIO_WriteBit(Step_DIR_port,Step1_DIR_Pin,Step_DIR);
			TIM_SetCompare1(Step_TIM, 104-1);
			break;
		case Step2:
            GPIO_WriteBit(Step_DIR_port,Step2_DIR_Pin,Step_DIR);
			TIM_SetCompare2(Step_TIM, 104-1);
			break;
		default:
			break;
	}
}

/**
  * 函    数：关闭指定电机
  * 参    数：Step_num：Step1, Step2, Step3, Step4
  * 返 回 值：无
  */
void Step_OFF(Step Step_num)
{
	switch (Step_num) 
	{
		case Step1:
			TIM_SetCompare1(Step_TIM, 0);
			break;
		case Step2:
			TIM_SetCompare2(Step_TIM, 0);
			break;
		default:
			break;
	}
}

/**
  * 函    数：定时器中断函数
  * 参    数：无
  * 返 回 值：无
  */
/*
void TIM2_IRQHandler(void)   
{
	if (TIM_GetITStatus(Step_TIM, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
	{
		TIM_ClearITPendingBit(Step_TIM, TIM_IT_Update);  //清除TIMx的中断待处理位:TIM 中断源 

	}
}
*/      
