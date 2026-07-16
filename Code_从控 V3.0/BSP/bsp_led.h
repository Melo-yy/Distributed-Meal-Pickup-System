#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "bsp_sys.h"

// 闪烁控制结构
typedef struct {
    uint8_t is_blinking;
    uint32_t interval_ms;
    uint32_t last_toggle_time;
    uint8_t current_state; // 0: off, 1: on
    uint8_t blink_red;
    uint8_t blink_green;
    uint8_t blink_blue;
} slave_led_blink_ctrl_t;

/*--------从控LED类型定义--------*/
typedef enum {
    SLAVE_LED_POWER = 0,    // 从控电源兼通信指示灯
    SLAVE_LED_MAX
} Slave_LED_Type_t;

/*--------从控LED定义--------*/
// 从控LED: RGB电源兼通信指示灯 (请根据实际硬件连接修改引脚)
#define SLAVE_LED_GPIO_CLK      RCC_APB2Periph_GPIOA
#define SLAVE_LED_GPIO_PORT     GPIOA
#define SLAVE_LED_RED_PIN       GPIO_Pin_4
#define SLAVE_LED_GREEN_PIN     GPIO_Pin_5	
#define SLAVE_LED_BLUE_PIN      GPIO_Pin_6

/*--------LED控制宏定义--------*/
// 从控LED控制 (根据实际硬件，高电平点亮或低电平点亮)
// 假设为高电平点亮
#define SLAVE_LED_RED_ON()      GPIO_SetBits(SLAVE_LED_GPIO_PORT, SLAVE_LED_RED_PIN)
#define SLAVE_LED_RED_OFF()     GPIO_ResetBits(SLAVE_LED_GPIO_PORT, SLAVE_LED_RED_PIN)
#define SLAVE_LED_RED_TOGGLE()  SLAVE_LED_GPIO_PORT->ODR ^= SLAVE_LED_RED_PIN
#define SLAVE_LED_GREEN_ON()    GPIO_SetBits(SLAVE_LED_GPIO_PORT, SLAVE_LED_GREEN_PIN)
#define SLAVE_LED_GREEN_OFF()   GPIO_ResetBits(SLAVE_LED_GPIO_PORT, SLAVE_LED_GREEN_PIN)
#define SLAVE_LED_GREEN_TOGGLE() SLAVE_LED_GPIO_PORT->ODR ^= SLAVE_LED_GREEN_PIN
#define SLAVE_LED_BLUE_ON()     GPIO_SetBits(SLAVE_LED_GPIO_PORT, SLAVE_LED_BLUE_PIN)
#define SLAVE_LED_BLUE_OFF()    GPIO_ResetBits(SLAVE_LED_GPIO_PORT, SLAVE_LED_BLUE_PIN)
#define SLAVE_LED_BLUE_TOGGLE() SLAVE_LED_GPIO_PORT->ODR ^= SLAVE_LED_BLUE_PIN

/*--------函数声明--------*/
void bsp_LED_Slave_Init(void);
void bsp_LED_Slave_SetColor(Slave_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue);
void bsp_LED_Slave_SetSolidColor(Slave_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue);
void bsp_LED_Slave_SetBlinkColor(Slave_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue, uint32_t interval_ms);
void bsp_LED_Slave_UpdateBlink(void);


#endif
