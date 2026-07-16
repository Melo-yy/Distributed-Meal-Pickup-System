#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "bsp_sys.h"

/*--------Host LED类型定义--------*/
typedef enum {
    HOST_LED_4G = 0,    // 4G通信状态指示灯
    HOST_LED_MAX
} Host_LED_Type_t;

//#define LED_COMMON_ANODE  // 注释掉这行就是共阴极

/*--------Host装置LED定义--------*/
// Host LED: 4G通信状态指示灯
#define HOST_4G_GPIO_CLK      RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOE
#define HOST_4G_GPIO_PORT     GPIOB
#define HOST_4G_RED_PIN       GPIO_Pin_0
#define HOST_4G_GREEN_PIN     GPIO_Pin_1
#define HOST_4G_BLUE_PIN_PORT GPIOE
#define HOST_4G_BLUE_PIN      GPIO_Pin_7

/*--------LED控制宏定义--------*/
// Host 4G指示灯控制
// 共阳极版本
#ifdef LED_COMMON_ANODE
    #define HOST_4G_RED_ON()      GPIO_ResetBits(HOST_4G_GPIO_PORT, HOST_4G_RED_PIN)
    #define HOST_4G_RED_OFF()     GPIO_SetBits(HOST_4G_GPIO_PORT, HOST_4G_RED_PIN)
    #define HOST_4G_GREEN_ON()    GPIO_ResetBits(HOST_4G_GPIO_PORT, HOST_4G_GREEN_PIN)
    #define HOST_4G_GREEN_OFF()   GPIO_SetBits(HOST_4G_GPIO_PORT, HOST_4G_GREEN_PIN)
    #define HOST_4G_BLUE_ON()     GPIO_ResetBits(HOST_4G_BLUE_PIN_PORT, HOST_4G_BLUE_PIN)
    #define HOST_4G_BLUE_OFF()    GPIO_SetBits(HOST_4G_BLUE_PIN_PORT, HOST_4G_BLUE_PIN)
#else
    // 共阴极版本（原来的）
    #define HOST_4G_RED_ON()      GPIO_SetBits(HOST_4G_GPIO_PORT, HOST_4G_RED_PIN)
    #define HOST_4G_RED_OFF()     GPIO_ResetBits(HOST_4G_GPIO_PORT, HOST_4G_RED_PIN)
    #define HOST_4G_GREEN_ON()    GPIO_SetBits(HOST_4G_GPIO_PORT, HOST_4G_GREEN_PIN)
    #define HOST_4G_GREEN_OFF()   GPIO_ResetBits(HOST_4G_GPIO_PORT, HOST_4G_GREEN_PIN)
    #define HOST_4G_BLUE_ON()     GPIO_SetBits(HOST_4G_BLUE_PIN_PORT, HOST_4G_BLUE_PIN)
    #define HOST_4G_BLUE_OFF()    GPIO_ResetBits(HOST_4G_BLUE_PIN_PORT, HOST_4G_BLUE_PIN)
#endif

/*--------函数声明--------*/
void bsp_LED_Host_Init(void);
void bsp_LED_Host_SetColor(Host_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue);
void bsp_LED_Host_SetSolidColor(Host_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue);
void bsp_LED_Host_SetBlinkColor(Host_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue, uint32_t interval_ms);
void bsp_LED_Host_UpdateBlink(void);

#endif
