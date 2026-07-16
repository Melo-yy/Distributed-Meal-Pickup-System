#include "./key/bsp_key.h"  
#include "./delay/delay.h"
#include "./hx711/bsp_hx711.h"
#include "./usart/bsp_usart.h"


u32 HX711_Buffer;
u32 Weight_Maopi;
s32 Weight_Shiwu;

void Hx711_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(Hx711_SCK_GPIO_CLK | Hx711_DOUT_GPIO_CLK, ENABLE);	 
	//HX711_SCK
	GPIO_InitStructure.GPIO_Pin = Hx711_SCK_GPIO_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(Hx711_SCK_GPIO_PORT, &GPIO_InitStructure);				
	
	//HX711_DOUT
    GPIO_InitStructure.GPIO_Pin = Hx711_DOUT_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//
    GPIO_Init(Hx711_DOUT_GPIO_PORT, &GPIO_InitStructure);  
	
	GPIO_SetBits(Hx711_SCK_GPIO_PORT,Hx711_SCK_GPIO_PIN);					//
}

u32 HX711_Read(void)	//
{
	unsigned long count; 
	unsigned char i; 
	GPIO_WriteBit(Hx711_DOUT_GPIO_PORT,Hx711_DOUT_GPIO_PIN,Bit_SET);//  	HX711_DOUT=1; 
	Delay_us(1);
	GPIO_WriteBit(Hx711_SCK_GPIO_PORT,Hx711_SCK_GPIO_PIN,Bit_RESET);//			HX711_SCK=0; 
  count=0;	
  while(GPIO_ReadInputDataBit(Hx711_DOUT_GPIO_PORT,Hx711_DOUT_GPIO_PIN)==1);
  	for(i=0;i<24;i++)
	{ 
	  GPIO_WriteBit(Hx711_SCK_GPIO_PORT,Hx711_SCK_GPIO_PIN,Bit_SET);		//HX711_SCK=1;		
	  count=count<<1; 
		Delay_us(1);
		GPIO_WriteBit(Hx711_SCK_GPIO_PORT,Hx711_SCK_GPIO_PIN,Bit_RESET);		//HX711_SCK=0; 
	  	if(GPIO_ReadInputDataBit(Hx711_DOUT_GPIO_PORT,Hx711_DOUT_GPIO_PIN))
			{
				count++;
			}			 
		Delay_us(1);
	} 
 	GPIO_WriteBit(Hx711_SCK_GPIO_PORT,Hx711_SCK_GPIO_PIN,Bit_SET);			//HX711_SCK=1; 
    count=count^0x800000;
	Delay_us(1);
	GPIO_WriteBit(Hx711_SCK_GPIO_PORT,Hx711_SCK_GPIO_PIN,Bit_RESET);			//HX711_SCK=0;  
	return(count);
}

void Get_Maopi(void)
{
	Weight_Maopi = HX711_Read();	
}

void Get_Weight(void)
{
	u8 i;
	for(i=0;i<5;i++)
	{
		HX711_Buffer=0;
		HX711_Buffer = HX711_Read();
		if(HX711_Buffer > Weight_Maopi)			
		{
			Weight_Shiwu = HX711_Buffer;
			Weight_Shiwu = Weight_Shiwu - Weight_Maopi;				
			Weight_Shiwu = (s32)(((float)Weight_Shiwu/GapValue));//5.415; 
		}
	}
}
