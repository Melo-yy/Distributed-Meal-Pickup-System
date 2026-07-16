#ifndef __BSP_KEY_H
#define __BSP_KEY_H	 

#include "bsp_sys.h"
#include "bsp_delay.h"


// 按键GPIO定义
#define KEY_GPIO_PORT    GPIOE
#define KEY_GPIO_PIN     GPIO_Pin_0
#define KEY_GPIO_CLK     RCC_AHB1Periph_GPIOE
#define KEY			PEin(0) 


// 按键状态定义
#define KEY_ON          0
#define KEY_OFF         1


// 按键事件枚举
typedef enum {
    KEY_EVENT_NONE = 0,   // 无事件
    KEY_EVENT_CLICK,      // 单击
    KEY_EVENT_DOUBLE,     // 双击
    KEY_EVENT_LONG_PRESS, // 长按(3秒)
    KEY_EVENT_LONG_HOLD   // 长按保持
} Key_Event_t;


// 函数声明
void bsp_KEY_Init(void);
Key_Event_t bsp_Key_Scan(void);
uint8_t bsp_Key_Read(void);




#endif 
