#ifndef __APP_LED_H
#define __APP_LED_H

#include "bsp_led.h"
#include "bsp_delay.h"
#include "bsp_usart.h"
#include "app_4g.h"

#define LED_TASK_PRIO		2     // 任务优先级
#define LED_STK_SIZE 		128   // 任务堆栈大小

extern QueueHandle_t xOrderProc4G_LED_Queue;  //4G模块--LEd任务队列

// LED控制结构体类型
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t is_blink;
    uint32_t blink_interval;
    const char* description;
} led_config_t;

/*--------电源/4G通信状态枚举--------*/
typedef enum {
    LED_4G_DISCONNECTED = 0,      // 蓝色 - 无法通信
    LED_4G_CONNECTING,            // 蓝色闪烁 - 通信连接尝试
    LED_4G_NORMAL,                // 绿色 - 通信正常
    LED_4G_DATA_TRANSFER,         // 绿色闪烁 - 数据传输中
    LED_4G_STATE_COUNT
} led_4g_state_t;



/*--------函数声明--------*/
void app_LED_Init(void);
void app_LED_Update(void);

// 简化的状态设置函数
void LED_Set4GState(led_4g_state_t state);


// 状态描述获取函数
const char* LED_Get4GStateDesc(void);


/*--------LED任务函数--------*/
void led_task(void *pvParameters);

#endif
