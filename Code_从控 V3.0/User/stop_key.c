#include "./stop_key.h"

//ÂÆö‰πâÂõõ‰∏™ÁîµÊú∫ÁöÑÊåâÈîÆÔºåÊØè‰∏™ÁîµÊú∫Êúâ‰∏§‰∏™ÊåâÈîÆ
KEY_STATE Step1_Key,Step2_Key,Step3_Key,Step4_Key;


/********************************************************************************/
void Stop_Key_TIM_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitSturcture;

    RCC_APB1PeriphClockCmd(Step_Key_TIM_CLK, ENABLE);           // πƒ‹∂® ±∆˜6 ±÷”

    /* ≈‰÷√TIM6 */
    TIM_TimeBaseInitStructure.TIM_Period = Step_Key_TIM_Period;                     //…Ë÷√◊‘∂Ø◊∞‘ÿ
    TIM_TimeBaseInitStructure.TIM_Prescaler = Step_Key_TIM_Prescaler;                 //…Ë÷√‘§∑÷∆µ
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;		//µ›‘ˆº∆ ˝
    TIM_TimeBaseInit(Step_Key_TIM, &TIM_TimeBaseInitStructure);								//≥ı ºªØTIM6Õ‚…Ë

    TIM_ITConfig(Step_Key_TIM, TIM_IT_Update, ENABLE);                     		// πƒ‹TIM6∏¸–¬÷–∂œ

    /* NVIC≈‰÷√ */
    NVIC_InitSturcture.NVIC_IRQChannel = Step_Key_TIM_IRQn;                		//TIM6÷–∂œ«Î«Û
    NVIC_InitSturcture.NVIC_IRQChannelPreemptionPriority = 1;      		//«¿’º”≈œ»º∂Œ™1
    NVIC_InitSturcture.NVIC_IRQChannelSubPriority = 2;             		//œÏ”¶”≈œ»º∂Œ™1
    NVIC_InitSturcture.NVIC_IRQChannelCmd = ENABLE;                		// πƒ‹÷–∂œ
    NVIC_Init(&NVIC_InitSturcture);                                		//≥ı ºªØNVIC

    TIM_Cmd(Step_Key_TIM, ENABLE);                                         		// πƒ‹TIM6
}

/********************************************************************************/
void Stop_Key_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(Step_Key_CLK,ENABLE);
	
	/*****************key0 key2 key3 key4*******************/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = Step1_Key_1_Pin | Step1_Key_2_Pin | Step2_Key_1_Pin | Step2_Key_2_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_Key_port, &GPIO_InitStructure);
	
	/*****************key0 key2 key3 key4*******************/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = Step3_Key_1_Pin | Step3_Key_2_Pin | Step4_Key_1_Pin | Step4_Key_2_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Step_Key_port, &GPIO_InitStructure);
	
	Stop_Key_TIM_Init();
}



void TIM6_IRQHandler(void)
{
	
		/****************************************Step1_key_1********************************************************/
	if(Step1_Key.flag1 == 0 || Step1_Key.flag1 == 3)
	{
		if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_1_Pin) == 0 && Step1_Key.bit1 == 0)			//KEY0 ◊¥Œ±ª∞¥œ¬
		{
			Step1_Key.bit1 = 1;
			Step1_Key.cnt1 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_1_Pin) == 0 && Step1_Key.bit1 == 1)		//œ˚∂∂Ω◊∂Œ
		{
			Step1_Key.cnt1++;
			if(Step1_Key.cnt1 == 3)
			{
				Step1_Key.cnt1 = 0;
				Step1_Key.bit1 = 2;
			}
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_1_Pin) == 1 && Step1_Key.bit1 == 1)		//∂∂∂Ø∏…»≈
		{
			Step1_Key.cnt1 = 0;
			Step1_Key.bit1 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_1_Pin) == 0 && Step1_Key.bit1 == 2)		//ºÏ≤‚ ◊¥Œ∞¥œ¬ ±≥§
		{
			Step1_Key.cnt1 = 0;
			Step1_Key.bit1 = 5;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_1_Pin) == 0 && Step1_Key.bit1 == 5)
		{
			Step1_Key.cnt1 = 0;
			Step1_Key.bit1 = 0;
			Step1_Key.flag1 = 3;		//≥§∞¥
		}
		else
		{
			Step1_Key.flag1 = 0;
		}
	}
		/****************************************Step1_key_2********************************************************/
	if(Step1_Key.flag2 == 0 || Step1_Key.flag2 == 3)
	{
		if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_2_Pin) == 0 && Step1_Key.bit2 == 0)			//KEY0 ◊¥Œ±ª∞¥œ¬
		{
			Step1_Key.bit2 = 1;
			Step1_Key.cnt2 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_2_Pin) == 0 && Step1_Key.bit2 == 1)		//œ˚∂∂Ω◊∂Œ
		{
			Step1_Key.cnt2++;
			if(Step1_Key.cnt2 == 3)
			{
				Step1_Key.cnt2 = 0;
				Step1_Key.bit2 = 2;
			}
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_2_Pin) == 1 && Step1_Key.bit2 == 1)		//∂∂∂Ø∏…»≈
		{
			Step1_Key.cnt2 = 0;
			Step1_Key.bit2 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_2_Pin) == 0 && Step1_Key.bit2 == 2)		//ºÏ≤‚ ◊¥Œ∞¥œ¬ ±≥§
		{
			Step1_Key.cnt2 = 0;
			Step1_Key.bit2 = 5;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step1_Key_2_Pin) == 0 && Step1_Key.bit2 == 5)
		{
			Step1_Key.cnt2 = 0;
			Step1_Key.bit2 = 0;
			Step1_Key.flag2 = 3;		//≥§∞¥
		}
		else
		{
			Step1_Key.flag2 = 0;
		}	
	}
		/****************************************Step2_key_1********************************************************/
	if(Step2_Key.flag1 == 0 || Step2_Key.flag1 == 3)
	{
		if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_1_Pin) == 0 && Step2_Key.bit1 == 0)			//KEY0 ◊¥Œ±ª∞¥œ¬
		{
			Step2_Key.bit1 = 1;
			Step2_Key.cnt1 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_1_Pin) == 0 && Step2_Key.bit1 == 1)		//œ˚∂∂Ω◊∂Œ
		{
			Step2_Key.cnt1++;
			if(Step2_Key.cnt1 == 3)
			{
				Step2_Key.cnt1 = 0;
				Step2_Key.bit1 = 2;
			}
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_1_Pin) == 1 && Step2_Key.bit1 == 1)		//∂∂∂Ø∏…»≈
		{
			Step2_Key.cnt1 = 0;
			Step2_Key.bit1 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_1_Pin) == 0 && Step2_Key.bit1 == 2)		//ºÏ≤‚ ◊¥Œ∞¥œ¬ ±≥§
		{
			Step2_Key.cnt1 = 0;
			Step2_Key.bit1 = 5;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_1_Pin) == 0 && Step2_Key.bit1 == 5)
		{
			Step2_Key.cnt1 = 0;
			Step2_Key.bit1 = 0;
			Step2_Key.flag1 = 3;		//≥§∞¥
		}
		else
		{
			Step2_Key.flag1 = 0;
		}
	}
		/****************************************Step2_key_2********************************************************/
	if(Step2_Key.flag2 == 0 || Step2_Key.flag2 == 3)
	{
		if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_2_Pin) == 0 && Step2_Key.bit2 == 0)			//KEY0 ◊¥Œ±ª∞¥œ¬
		{
			Step2_Key.bit2 = 1;
			Step2_Key.cnt2 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_2_Pin) == 0 && Step2_Key.bit2 == 1)		//œ˚∂∂Ω◊∂Œ
		{
			Step2_Key.cnt2++;
			if(Step2_Key.cnt2 == 3)
			{
				Step2_Key.cnt2 = 0;
				Step2_Key.bit2 = 2;
			}
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_2_Pin) == 1 && Step2_Key.bit2 == 1)		//∂∂∂Ø∏…»≈
		{
			Step2_Key.cnt2 = 0;
			Step2_Key.bit2 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_2_Pin) == 0 && Step2_Key.bit2 == 2)		//ºÏ≤‚ ◊¥Œ∞¥œ¬ ±≥§
		{
			Step2_Key.cnt2 = 0;
			Step2_Key.bit2 = 5;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step2_Key_2_Pin) == 0 && Step2_Key.bit2 == 5)
		{
			Step2_Key.cnt2 = 0;
			Step2_Key.bit2 = 0;
			Step2_Key.flag2 = 3;		//≥§∞¥
		}
		else
		{
			Step2_Key.flag2 = 0;
		}	
	}
		/****************************************Step3_key_1********************************************************/
	if(Step3_Key.flag1 == 0 || Step3_Key.flag1 == 3)
	{
		if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_1_Pin) == 0 && Step3_Key.bit1 == 0)			//KEY0 ◊¥Œ±ª∞¥œ¬
		{
			Step3_Key.bit1 = 1;
			Step3_Key.cnt1 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_1_Pin) == 0 && Step3_Key.bit1 == 1)		//œ˚∂∂Ω◊∂Œ
		{
			Step3_Key.cnt1++;
			if(Step3_Key.cnt1 == 3)
			{
				Step3_Key.cnt1 = 0;
				Step3_Key.bit1 = 2;
			}
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_1_Pin) == 1 && Step3_Key.bit1 == 1)		//∂∂∂Ø∏…»≈
		{
			Step3_Key.cnt1 = 0;
			Step3_Key.bit1 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_1_Pin) == 0 && Step3_Key.bit1 == 2)		//ºÏ≤‚ ◊¥Œ∞¥œ¬ ±≥§
		{
			Step3_Key.cnt1 = 0;
			Step3_Key.bit1 = 5;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_1_Pin) == 0 && Step3_Key.bit1 == 5)
		{
			Step3_Key.cnt1 = 0;
			Step3_Key.bit1 = 0;
			Step3_Key.flag1 = 3;		//≥§∞¥
		}
		else
		{
			Step3_Key.flag1 = 0;
		}
	}
		/****************************************Step3_Key_2********************************************************/
	if(Step3_Key.flag2 == 0 || Step3_Key.flag2 == 3)
	{
		if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_2_Pin) == 0 && Step3_Key.bit2 == 0)			//KEY0 ◊¥Œ±ª∞¥œ¬
		{
			Step3_Key.bit2 = 1;
			Step3_Key.cnt2 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_2_Pin) == 0 && Step3_Key.bit2 == 1)		//œ˚∂∂Ω◊∂Œ
		{
			Step3_Key.cnt2++;
			if(Step3_Key.cnt2 == 3)
			{
				Step3_Key.cnt2 = 0;
				Step3_Key.bit2 = 2;
			}
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_2_Pin) == 1 && Step3_Key.bit2 == 1)		//∂∂∂Ø∏…»≈
		{
			Step3_Key.cnt2 = 0;
			Step3_Key.bit2 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_2_Pin) == 0 && Step3_Key.bit2 == 2)		//ºÏ≤‚ ◊¥Œ∞¥œ¬ ±≥§
		{
			Step3_Key.cnt2 = 0;
			Step3_Key.bit2 = 5;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step3_Key_2_Pin) == 0 && Step3_Key.bit2 == 5)
		{
			Step3_Key.cnt2 = 0;
			Step3_Key.bit2 = 0;
			Step3_Key.flag2 = 3;		//≥§∞¥
		}
		else
		{
			Step3_Key.flag2 = 0;
		}	
	}
	/****************************************Step4_key_1********************************************************/
	if(Step4_Key.flag1 == 0 || Step4_Key.flag1 == 3)
	{
		if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_1_Pin) == 0 && Step4_Key.bit1 == 0)			//KEY0 ◊¥Œ±ª∞¥œ¬
		{
			Step4_Key.bit1 = 1;
			Step4_Key.cnt1 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_1_Pin) == 0 && Step4_Key.bit1 == 1)		//œ˚∂∂Ω◊∂Œ
		{
			Step4_Key.cnt1++;
			if(Step4_Key.cnt1 == 3)
			{
				Step4_Key.cnt1 = 0;
				Step4_Key.bit1 = 2;
			}
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_1_Pin) == 1 && Step4_Key.bit1 == 1)		//∂∂∂Ø∏…»≈
		{
			Step4_Key.cnt1 = 0;
			Step4_Key.bit1 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_1_Pin) == 0 && Step4_Key.bit1 == 2)		//ºÏ≤‚ ◊¥Œ∞¥œ¬ ±≥§
		{
			Step4_Key.cnt1 = 0;
			Step4_Key.bit1 = 5;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_1_Pin) == 0 && Step4_Key.bit1 == 5)
		{
			Step4_Key.cnt1 = 0;
			Step4_Key.bit1 = 0;
			Step4_Key.flag1 = 3;		//≥§∞¥
		}
		else
		{
			Step4_Key.flag1 = 0;
		}
	}
	/****************************************Step4_key_2********************************************************/
	if(Step4_Key.flag2 == 0 || Step4_Key.flag2 == 3)
	{
		if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_2_Pin) == 0 && Step4_Key.bit2 == 0)			//KEY0 ◊¥Œ±ª∞¥œ¬
		{
			Step4_Key.bit2 = 1;
			Step4_Key.cnt2 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_2_Pin) == 0 && Step4_Key.bit2 == 1)		//œ˚∂∂Ω◊∂Œ
		{
			Step4_Key.cnt2++;
			if(Step4_Key.cnt2 == 3)
			{
				Step4_Key.cnt2 = 0;
				Step4_Key.bit2 = 2;
			}
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_2_Pin) == 1 && Step4_Key.bit2 == 1)		//∂∂∂Ø∏…»≈
		{
			Step4_Key.cnt2 = 0;
			Step4_Key.bit2 = 0;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_2_Pin) == 0 && Step4_Key.bit2 == 2)		//ºÏ≤‚ ◊¥Œ∞¥œ¬ ±≥§
		{
			Step4_Key.cnt2 = 0;
			Step4_Key.bit2 = 5;
		}
		else if(GPIO_ReadInputDataBit(Step_Key_port,Step4_Key_2_Pin) == 0 && Step4_Key.bit2 == 5)
		{
			Step4_Key.cnt2 = 0;
			Step4_Key.bit2 = 0;
			Step4_Key.flag2 = 3;		//≥§∞¥
		}
		else
		{
			Step4_Key.flag2 = 0;
		}	
	}
	TIM_ClearITPendingBit(TIM6,TIM_IT_Update);
}
