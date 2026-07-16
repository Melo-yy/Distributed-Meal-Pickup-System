#ifndef __HX711_H
#define	__HX711_H


#include "stm32f10x.h"

//  SCK引脚定义
#define    Hx711_SCK_GPIO_CLK     RCC_APB2Periph_GPIOB
#define    Hx711_SCK_GPIO_PORT    GPIOB			   
#define    Hx711_SCK_GPIO_PIN	 		GPIO_Pin_0

//  DOUT引脚定义
#define    Hx711_DOUT_GPIO_CLK     RCC_APB2Periph_GPIOB
#define    Hx711_DOUT_GPIO_PORT    GPIOB			   
#define    Hx711_DOUT_GPIO_PIN	 	 GPIO_Pin_1

//	校准参数
#define 	 GapValue 2110

extern u32 HX711_Buffer;
extern u32 Weight_Maopi;
extern s32 Weight_Shiwu;
extern u8  Flag_Error;


void Hx711_Init(void);
u32 HX711_Read(void);
void Get_Maopi(void);
void Get_Weight(void);


#endif /* __HX711_H */

