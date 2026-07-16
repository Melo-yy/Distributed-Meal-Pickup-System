#include "./step/bsp_step.h"                  // Device header
//#include "./usart/bsp_usart.h"
#include "./delay/delay.h"




//u8 STEP1_flag,STEP2_flag;				//步进电机1的启动标志，步进电机2的启动标志（1启动，0关闭）
//u8 Step_Switch1_flag,Step_Switch2_flag,Step_Switch3_flag,Step_Switch4_flag;//四个微动开关的标志（1被触发）

void STEP_Init(void)			//共地，电机与微动开关初始化，参数默认(158-1,72-1)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB1PeriphClockCmd(Step_TIM_CLK, ENABLE);
	RCC_APB2PeriphClockCmd(Step_PUL_CLK | Step_DIR_CLK | Step_ENA_CLK, ENABLE);
	
	//脉冲引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = Step1_Pul_Pin | Step2_Pul_Pin ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_PUL_port, &GPIO_InitStructure);
	
	//方向引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = Step1_DIR_Pin | Step2_DIR_Pin ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_DIR_port, &GPIO_InitStructure);
	
	//使能引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = Step1_ENA_Pin | Step2_ENA_Pin ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_ENA_port, &GPIO_InitStructure);
		
	TIM_InternalClockConfig(TIM2);//开启定时器2中断
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = Step_TIM_Period;
	TIM_TimeBaseInitStructure.TIM_Prescaler = Step_TIM_Prescaler;		
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(Step_TIM, &TIM_TimeBaseInitStructure);
	
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;		//CCR
	
	TIM_ClearFlag(Step_TIM, TIM_FLAG_Update);
	TIM_ITConfig(Step_TIM,TIM_IT_Update,ENABLE ); 
	
	//中断设置
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);  
	
	TIM_OC1Init(Step_TIM, &TIM_OCInitStructure);
	TIM_OC2Init(Step_TIM, &TIM_OCInitStructure);
	TIM_OC3Init(Step_TIM, &TIM_OCInitStructure);
	TIM_OC4Init(Step_TIM, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(Step_TIM, TIM_OCPreload_Enable); 
	TIM_OC2PreloadConfig(Step_TIM, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(Step_TIM, TIM_OCPreload_Enable); 
	TIM_OC4PreloadConfig(Step_TIM, TIM_OCPreload_Enable);	
	TIM_Cmd(Step_TIM, ENABLE);
	
	GPIO_WriteBit(Step_ENA_port,Step1_ENA_Pin,Step_ENA_OFF);
	GPIO_WriteBit(Step_ENA_port,Step2_ENA_Pin,Step_ENA_OFF);

	
	//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//微动开关
	//	GPIO_InitStructure.GPIO_Pin = Step_Switch1_Pin | Step_Switch2_Pin | Step_Switch3_Pin | Step_Switch4_Pin;
	//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	//	GPIO_Init(Step_Switch_port, &GPIO_InitStructure);
}


/**************************
* 函数名称：	Step_ON
* 参数：			Step_num：Step1, Step2, Step3, Step4
* 参数：			Step_DIR：Step_DIR_Forward, Step_DIR_Counter
* 返回值：		无
* 功能：			开启电机
***************************/
void Step_ON(Step Step_num, BitAction Step_DIR)
{
	switch (Step_num) 
	{
		case Step1:
			GPIO_WriteBit(Step_ENA_port,Step1_ENA_Pin,Step_ENA_ON);
			GPIO_WriteBit(Step_DIR_port,Step1_DIR_Pin,Step_DIR);
			TIM_SetCompare1(Step_TIM, 104-1);
			break;
		case Step2:
			GPIO_WriteBit(Step_ENA_port,Step2_ENA_Pin,Step_ENA_ON);
			GPIO_WriteBit(Step_DIR_port,Step2_DIR_Pin,Step_DIR);
			TIM_SetCompare2(Step_TIM, 104-1);
			break;
		default:
			break;
	}
}

/**************************
* 函数名称：	Step_OFF
* 参数：			Step_num：Step1, Step2, Step3, Step4
* 返回值：		无
* 功能：			关闭电机
***************************/
void Step_OFF(Step Step_num)
{
	switch (Step_num) 
	{
		case Step1:
			GPIO_WriteBit(Step_ENA_port,Step1_ENA_Pin,Step_ENA_OFF);
			TIM_SetCompare1(Step_TIM, 0);
			break;
		case Step2:
			GPIO_WriteBit(Step_ENA_port,Step2_ENA_Pin,Step_ENA_OFF);
			TIM_SetCompare2(Step_TIM, 0);
			break;
		default:
			break;
	}
}



//void TIM2_IRQHandler(void)   
//{
//	if (TIM_GetITStatus(Step_TIM, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
//	{
//		TIM_ClearITPendingBit(Step_TIM, TIM_IT_Update  );  //清除TIMx的中断待处理位:TIM 中断源 

//	}
//}





