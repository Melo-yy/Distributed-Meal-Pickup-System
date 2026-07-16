#include "stm32f10x.h"                  // Device header
#include "Motor.h"
#include "USART1.h"
#include "PWM.h"
#include "R200.h"
#include "./Key/bsp_key.h"
#include "./step/bsp_step.h"
#include "./delay/delay.h"
#include "LED.h"

#define KEY_ON	1
#define KEY_OFF	0

   int  push_egg_end=0 ;  
   int  push_egg_start = 0;  
   int  push_plate_end = 0;  
   int  push_plate_start = 0;  
   int  flexible_up = 0;  
   int  flexible_down = 0; 

void Init_switchdata(void) {  
   int  push_egg_end = 0;  
   int  push_egg_start = 0;  
   int  push_plate_end = 0;  
   int  push_plate_start = 0;  
   int  flexible_up = 0;  
   int  flexible_down = 0;  
}

extern int E_left ;

extern  int  push_egg_end ;  
extern   int  push_egg_start;  
extern   int  push_plate_end;  
extern   int  push_plate_start;  
extern   int  flexible_up ;  
extern   int  flexible_down ; 


void SWITCH_Init()
{
    /*开启时钟*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure1;
    GPIO_InitStructure1.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure1.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6  | GPIO_Pin_8 | GPIO_Pin_9 ;
    GPIO_InitStructure1.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure1);
    
    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_6  | GPIO_Pin_8 | GPIO_Pin_9 );
	

    
    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure2;
    GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure2.GPIO_Pin =  GPIO_Pin_5 | GPIO_Pin_4;
    GPIO_InitStructure2.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure2);
    
    GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_4 );   
}

void KEY_Init(void)
{
 	GPIO_InitTypeDef GPIO_InitStructure;
 
 	RCC_APB2PeriphClockCmd(KEY2_GPIO_CLK|KEY3_GPIO_CLK,ENABLE);//使能PORTA,PORTE时钟


	GPIO_InitStructure.GPIO_Pin  = KEY2_GPIO_PIN;//KEY0-KEY1
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStructure);
	
	//初始化 WK_UP-->GPIOA.0	  下拉输入
	GPIO_InitStructure.GPIO_Pin  = KEY3_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //PA0设置成输入，默认下拉	  
	GPIO_Init(KEY3_GPIO_PORT, &GPIO_InitStructure);

} 


void Key_Init(void)  
{  
    GPIO_InitTypeDef GPIO_InitStructure;  

    // 使能 GPIOA 和 GPIOB 的时钟  
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);   

    // 初始化 GPIOB, GPIO_Pin_9 为上拉输入  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;  
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 设置为上拉输入模式  
    GPIO_Init(GPIOB, &GPIO_InitStructure);  

    // 初始化 GPIOB, GPIO_Pin_8 为上拉输入  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;  
    GPIO_Init(GPIOB, &GPIO_InitStructure);  

    // 初始化 GPIOB, GPIO_Pin_5 为上拉输入  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;  
    GPIO_Init(GPIOB, &GPIO_InitStructure);  
    
    // 初始化 GPIOB, GPIO_Pin_6 为上拉输入  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;  
    GPIO_Init(GPIOB, &GPIO_InitStructure);  
    
    // 初始化 GPIOA, GPIO_Pin_4 为上拉输入  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);  

    // 初始化 GPIOA, GPIO_Pin_5 为上拉输入  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;  
    GPIO_Init(GPIOA, &GPIO_InitStructure);  
} 



uint8_t Key_Scan(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin)
{			
	/*检测是否有按键按下*/
	if(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) == KEY_ON )  
	{	 
//		delay_ms(10);
		/*等待按键释放 */
//		while(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) == KEY_ON); 
		return 	KEY_ON;	 
	}
	else
		return KEY_OFF;
}



//int Switch_flexible_up_egg()
//{
//	
//	  R200_Send(1);
//        
//    if(USART1_GetRxFlag() == 1)
//        {
//            if(USART1_RxDataArray[1] == 0x02 && USART1_RxDataArray[2] == 0x22)  
//            {
//                if(R200_Check(USART1_RxDataArray + 7) == 1) //判断EPC码
//                {
//                    while(R200_Check(USART1_RxDataArray + 7) == 0);
//										Motor_SetNegative();
//									  
//                }
//            }
//        }

//}//等鹅离开之后，伸缩杆收缩

void Switch_flexible_up_egg()
{
	        R200_Send(1);
        
        if(USART1_GetRxFlag() == 1)
        {
            if(USART1_RxDataArray[1] == 0x02 && USART1_RxDataArray[2] == 0x22)  
            {
                if(R200_Check(USART1_RxDataArray + 7) == 0) //判断EPC码
                {

                    LED_ON();
									Delay_ms(500);
										E_left=1 ;	
									
                }
            }
        }
			
}





void Switch_flexible_down_egg()
{
	if(Key_Scan(GPIOA,GPIO_Pin_5))
	{
		Motor_SetPositive();		
	}

}//伸缩杆碰到下端沿，伸缩杆开始伸展




void Switch_flexible_stop_egg()
{
	if(Key_Scan(GPIOA,GPIO_Pin_4))
	{
		Motor_SetPositive();		
	}

}//伸缩杆碰到上端沿，伸缩杆开始停止









void IF_Switch_push_egg_end()
{

			if(Key_Scan(GPIOB,GPIO_Pin_5))
			
		{
			Step_ON(Step2,Step_DIR_Forward);
		}
		
}//推蛋碰到终点开关，开始往起点走


void IF_Switch_push_egg_start()
{

			if(Key_Scan(GPIOB,GPIO_Pin_6))
			
		{
			Step_OFF(Step2);
			push_egg_start=1;
		}		
}//推蛋碰到开始开关，步进电机2停止运行



void IF_Switch_start_push_plate_()
{
	if(push_egg_start)
	{
		Step_ON(Step1,Step_DIR_Counter);
		
	}
	
}//标志位push_egg_start代表推蛋装置回到原点，补盘开始推动





void IF_Switch_push_plate_end()
	
{
			if(Key_Scan(GPIOB,GPIO_Pin_8) )
			
		{
			push_egg_start=0;
			Step_ON(Step1,Step_DIR_Forward);
		}
		
}//补盘到达终点，步进电机开始向起点旋转




void IF_Switch_push_plate_start()
	
{
			if(Key_Scan(GPIOB,GPIO_Pin_9) )
			
		{
			Step_OFF(Step1);
		}
		
}//补盘回到起点，步进电机1停止转动



//void Switch_end_push_egg()
//{
//		if()
//	
//}



