#ifndef __APP_VOICE_H
#define __APP_VOICE_H

#include "bsp_voice.h"	
#include "app_scan.h"
#include "app_can.h"
#include "app_4g.h"

#define VOICE_TASK_PRIO		3
#define VOICE_STK_SIZE 		512  

// 语音重复播放的最小间隔时间（毫秒）
#define VOICE_REPEAT_INTERVAL_MS    8000  // 8秒间隔

// 同一状态最大重复播放次数
#define VOICE_MAX_REPEAT_COUNT        2   // 最多重复2次

// 语音管理器结构
typedef struct {
    system_state_t current_state;      // 当前状态
    system_state_t previous_state;     // 先前状态
    uint32_t state_change_time;        // 状态改变时间
    uint8_t pickup_quantity;           // 存储取餐份数
    SemaphoreHandle_t state_mutex;     // 状态互斥锁
} voice_manager_t;


extern QueueHandle_t xHostVoiceQueue;            // 语音任务队列
extern QueueHandle_t xVoiceQueue;			 	 // 扫描模块--语音任务队列
extern QueueHandle_t xOrderProc4G_Voice_Queue;   // 4G模块--语音任务队列

// 函数声明
void app_voice_manager_init(void);
void app_voice_set_state(system_state_t new_state);
void app_voice_set_state_with_quantity(system_state_t new_state, uint8_t quantity);
system_state_t app_voice_get_current_state(void);
void app_voice_play_by_state(void);
void app_voice_content_process(void);
void voice_task(void *pvParameters);

#endif
