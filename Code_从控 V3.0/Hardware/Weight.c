//#include "stm32f10x.h"                  // Device header
//#include "HX711.h"
//#include "LED.h"
//#include <stdio.h>

//#define GapValue 400




//unsigned int Flag_ERROR;

///**
//   * @brief	 Weight压力传感器获取毛皮
//   * @param  无
//   * @retval Weight_Maopi 毛皮
//   */
//unsigned long Get_Maopi()
//{
//			
//	unsigned long Maopi=0;
//	Maopi = HX711_ReadCommand();
////	printf("Maopi:%d\r\n", Maopi);
//    return Maopi;
//}

///**
//   * @brief	 Weight压力传感器获取净重
//   * @param  无
//   * @retval Weight_Shiwu 净重
//   */
//long Get_Weight(unsigned long Maopi)
//{
//	long Shiwu=0;
//	Shiwu = HX711_ReadCommand();
//	Shiwu -= Maopi;		//获取净重
//	if( Shiwu > 0)			
//	{	
//		Shiwu = (unsigned int)((float)Shiwu/GapValue); 	//计算实物的实际重量
//		if(Shiwu > 5000)		//超重报警
//            Flag_ERROR=1;	
//		else
//            Flag_ERROR=0;
//	}
//	else
//	{
//		Shiwu = 0;
//        Flag_ERROR = 1;
//	}
//    return Shiwu;
//}
