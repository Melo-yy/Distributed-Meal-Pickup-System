#ifndef __APP_SHOW_H
#define __APP_SHOW_H

#include <stdbool.h>
#include "bsp_show.h"
#include "app_can_common.h"
#include "app_can.h"
#include "main.h"
#include "debug_config.h"


#define SHOW_TASK_PRIO		3
#define SHOW_STK_SIZE 		256  

// 显示管理器结构
typedef struct {
    system_state_t current_state;			// 当前状态
    system_state_t previous_state;		// 先前状态
    uint32_t state_change_time;				// 状态改变时间
		uint8_t pickup_quantity;  				// 存储取餐份数
    SemaphoreHandle_t state_mutex;    // 状态互斥锁
} display_manager_t;

// 状态自动超时相关定义
#define APP_SHOW_STATE_AUTO_TIMEOUT_MS_DEFAULT    30000  // 默认30秒超时

extern QueueHandle_t xDisplayQueue;  			// 扫码模块的显示任务队列  
extern QueueHandle_t xSlaveDisplayQueue;	// CAN从机发送来的显示队列

// 函数声明
void 						app_show_manager_init(void);
void 						app_show_set_state(system_state_t new_state);
system_state_t 	app_show_get_current_state(void);
const char* 		app_show_get_state_description(system_state_t state);
void 						app_show_update_content(void);

void 						show_task(void *pvParameters);
void 						app_show_content_show(void);

// 状态自动超时相关函数声明
bool 						app_show_should_timeout_state(system_state_t state);
void 						app_show_set_timeout_ms(uint32_t timeout_ms);
uint32_t 				app_show_get_timeout_ms(void);

#endif
