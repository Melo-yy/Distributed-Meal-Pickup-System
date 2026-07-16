#include "stm32f10x.h"                  // Device header
#include "PWM.h"

/**
  * 函    数：直流电机初始化
  * 参    数：无
  * 返 回 值：无
  */
void Motor_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA6和PA7引脚初始化为推挽输出	
	
	PWM_Init();													//初始化直流电机的底层PWM
}


void Motor_SetPositive(void)                //伸杆
{
//		PWM_SetCompare2(100);
		GPIO_SetBits(GPIOA, GPIO_Pin_6);	//PA6置高电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_7);	//PA7置低电平，设置方向为正转
}

void Motor_SetNegative(void)                //缩杆
{
		PWM_SetCompare2	(100);	
		GPIO_ResetBits(GPIOA, GPIO_Pin_6);	//PA6置低电平
		GPIO_SetBits(GPIOA, GPIO_Pin_7);	//PA7置高电平，设置方向为反转
}

void Motor_SetOff(void)                     //停止
{
		PWM_SetCompare2	(100);
		GPIO_ResetBits(GPIOA, GPIO_Pin_6);	//PA6置低电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_7);	//PA7置低电平
}

void Motor_Set(void)   //正转反转切换
{
	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_6) == 0)		//如果PA6为低电平，也就是反转
	{
		PWM_SetCompare2	(100);
		GPIO_SetBits(GPIOA, GPIO_Pin_6);	//PA6置高电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_7);	//PA7置低电平，设置方向为正转
	}
	else									//否则，设置反转
	{
		PWM_SetCompare2	(100);
		GPIO_ResetBits(GPIOA, GPIO_Pin_6);	//PA6置低电平
		GPIO_ResetBits(GPIOA, GPIO_Pin_7);	//PA7置高电平，设置方向为反转
	}
}
