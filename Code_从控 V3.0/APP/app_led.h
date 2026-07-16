#ifndef __APP_LED_H
#define __APP_LED_H 			   

#include "bsp_led.h"
#include "main.h"

#define LED_TASK_PRIO		1     //Task priority //任务优先级
#define LED_STK_SIZE 		256   //Task stack size //任务堆栈大小

#define LED_SLAVE_STATE_COUNT (sizeof(slave_led_configs) / sizeof(slave_led_configs[0]))

extern QueueHandle_t xSlaveLEDQueue;

// 定义结构体类型
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t is_blink;
    uint16_t blink_interval;
    const char* description;
} slave_led_config_t;

/*--------从控LED状态枚举--------*/
typedef enum {
    LED_SLAVE_STATE_COMM_NORMAL = 0,    // 绿色：通信正常
    LED_SLAVE_STATE_DATA_TRANSFER,      // 绿色闪烁：数据传输中
    LED_SLAVE_STATE_NO_COMM,            // 蓝色：无法通信
    LED_SLAVE_STATE_COMM_ATTEMPT        // 蓝色闪烁：通信连接尝试
} led_slave_state_t;

/*--------函数声明--------*/
void app_LED_Slave_Init(void);
void app_LED_Slave_Update(void);
void LED_Slave_SetState(led_slave_state_t state);
const char* LED_Slave_GetStateDesc(void);

void led_task(void *pvParameters);

 
#endif





























