#ifndef __APP_SHOW_H
#define __APP_SHOW_H

#include "bsp_show.h"
#include "app_can.h"
#include "app_scan.h"
#include "app_4g.h"


#define SHOW_TASK_PRIO		3
#define SHOW_STK_SIZE 		512  

// 状态自动超时返回普通状态的时间（毫秒）
#define STATE_AUTO_TIMEOUT_MS    30000  // 30秒

// 显示管理器结构
typedef struct {
    system_state_t current_state;			// 当前状态
    system_state_t previous_state;		// 先前状态
    uint32_t state_change_time;				// 状态改变时间
		uint8_t pickup_quantity;  				// 存储取餐份数
    SemaphoreHandle_t state_mutex;    // 状态互斥锁
} display_manager_t;


extern QueueHandle_t xHostDisplayQueue;  // 主机CAN--显示任务队列  
extern QueueHandle_t xDisplayQueue;			 // 扫描模块--显示任务队列
extern QueueHandle_t xOrderProc4G_Show_Queue; //4G模块--显示任务队列

// 函数声明
void 						app_show_manager_init(void);
void 						app_show_set_state(system_state_t new_state);
system_state_t 	app_show_get_current_state(void);
const char* 		app_show_get_state_description(system_state_t state);
void 						app_show_update_content(void);
void 						app_show_set_timeout_ms(uint32_t timeout_ms);

void show_task(void *pvParameters);
void app_show_content_show(void);


#endif
