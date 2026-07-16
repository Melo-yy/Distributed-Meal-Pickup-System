#ifndef __APP_VOICE_H
#define __APP_VOICE_H

#include <stdbool.h>
#include "bsp_voice.h"	
#include "app_scan.h"
#include "app_can.h"
#include "semphr.h"   // 提供 SemaphoreHandle_t 定义
#include "debug_config.h"

#define VOICE_TASK_PRIO		3
#define VOICE_STK_SIZE 		256  

// 语音重复播放相关定义
#define VOICE_REPEAT_INTERVAL_MS      8000   // 语音重复播放最小间隔（8秒）
#define VOICE_MAX_REPEAT_COUNT        2      // 同状态最大播放次数（最多2次）

// 语音管理器结构
typedef struct {
    system_state_t current_state;      // 当前状态
    system_state_t previous_state;     // 先前状态
    uint8_t pickup_quantity;           // 存储取餐份数
    SemaphoreHandle_t state_mutex;     // 状态互斥锁
    // 语音重复播放相关字段
    uint32_t last_play_time;           // 上次播放时间
    uint32_t state_play_count;         // 当前状态播放次数
} voice_manager_t;


extern QueueHandle_t xSlaveVoiceQueue;    // 语音任务队列
extern QueueHandle_t xVoiceQueue;			 	 // 扫描模块--语音任务队列


// 函数声明
void app_voice_manager_init(void);
void app_voice_set_state(system_state_t new_state);
void app_voice_set_state_with_quantity(system_state_t new_state, uint8_t quantity);
system_state_t app_voice_get_current_state(void);
void app_voice_play_by_state(void);
void app_voice_content_process(void);
void voice_task(void *pvParameters);
#endif
