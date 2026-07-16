#ifndef __BSP_STEP_H
#define __BSP_STEP_H

#include "stm32f10x.h"                 // Device header


//步进电机定时器
#define Step_TIM_CLK 					RCC_APB1Periph_TIM2
#define Step_TIM 					    TIM2
#define Step_TIM_IRQChannel 	TIM2_IRQn
#define Step_TIM_Period				158-1	    //ARR
#define Step_TIM_Prescaler 		72-1			//PSC


//步进电机的时钟
#define Step_PUL_CLK 					RCC_APB2Periph_GPIOA
#define Step_DIR_CLK 					RCC_APB2Periph_GPIOB
#define Step_ENA_CLK 					RCC_APB2Periph_GPIOC

//步进电机的GPIO
#define Step_PUL_port 				GPIOA
#define Step_DIR_port 				GPIOB
#define Step_ENA_port 				GPIOC

//步进电机1
#define Step1_Pul_Pin 				GPIO_Pin_0
#define Step1_DIR_Pin 				GPIO_Pin_0
#define Step1_ENA_Pin 				GPIO_Pin_1

//步进电机2
#define Step2_Pul_Pin 				GPIO_Pin_1
#define Step2_DIR_Pin 				GPIO_Pin_1
#define Step2_ENA_Pin 				GPIO_Pin_3


//步进电机方向控制
#define 	Step_DIR_Forward 	    Bit_RESET               //顺时针
#define 	Step_DIR_Counter 			Bit_SET               //逆时针

//步进电机使能控制
#define 	Step_ENA_ON 	        Bit_RESET               //开启
#define 	Step_ENA_OFF 		   	  Bit_SET               //关闭

//步进电机的选择
typedef enum {
	Step1,
	Step2
}Step;




void STEP_Init(void);
void Step_ON(Step Step_num, BitAction Step_DIR);
void Step_OFF(Step Step_num);

extern u8 STEP1_flag,STEP2_flag;	

#endif
