#include "app_voice.h"

static voice_manager_t voice_manager;

/**************************************************************************
Function: app_voice_should_repeat_state
Input   : repeat_count (已播次数), elapsed_time_ms (距上次播放时间)
Output  : bool (是否允许播放)
函数功能：内部逻辑判断函数，决定当前是否满足“首次播放”或“满足间隔的重复播报”条件。
入口参数：repeat_count-当前状态下已成功下发指令的次数；
					elapsed_time_ms-自上次播报至今的时间。
返回  值：true-可以播报；false-应当保持沉默。
**************************************************************************/
static bool app_voice_should_repeat_state(uint8_t repeat_count, TickType_t elapsed_time_ms)
{
    // 首次播报，直接放行
    if (repeat_count == 0) {
        return true;
    }
    
    // 重复播报：未达到最大次数，且经过了规定的时间间隔
    if (repeat_count < VOICE_MAX_REPEAT_COUNT && elapsed_time_ms >= VOICE_REPEAT_INTERVAL_MS) {
        return true;
    }
   
    // 不满足条件，保持沉默
    return false;
}


/**************************************************************************
Function: app_voice_manager_init
Input   : none
Output  : none
函数功能：语音管理模块初始化。清零全局管理器结构体，设置初始状态为 NORMAL，并创建状态互斥锁。
入口参数：无。
返回  值：无。
**************************************************************************/
void app_voice_manager_init(void)
{
    memset(&voice_manager, 0, sizeof(voice_manager));
    voice_manager.current_state = STATE_NORMAL;
    voice_manager.previous_state = STATE_NORMAL;
    voice_manager.pickup_quantity = 0;  
    
    voice_manager.state_mutex = xSemaphoreCreateMutex();
    if (voice_manager.state_mutex == NULL) {
        DBG_ERROR("VOICE: Failed to create mutex!\n");
    }
}

/**************************************************************************
Function: app_voice_set_state
Input   : new_state (目标状态)
Output  : none
函数功能：线程安全地更新当前语音状态。若状态发生变化，将重置内部播放计数器和时间记录。
入口参数：new_state-系统识别到的最新状态枚举值。
返回  值：无。
**************************************************************************/
void app_voice_set_state(system_state_t new_state)
{
    if (xSemaphoreTake(voice_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (voice_manager.current_state != new_state) {
            voice_manager.previous_state = voice_manager.current_state;
            voice_manager.current_state = new_state;
            // 重置播放计数和时间，因为状态改变了
            voice_manager.state_play_count = 0;
            voice_manager.last_play_time = 0;
            
            DBG_INFO("VOICE: 状态变更 %d->%d\n", 
                   voice_manager.previous_state, voice_manager.current_state);
        }
        xSemaphoreGive(voice_manager.state_mutex);
    }
}


/**************************************************************************
Function: app_voice_set_state_with_quantity
Input   : new_state, quantity (餐品数量)
Output  : none
函数功能：app_voice_set_state 的扩展版，用于支持需要播报具体数值的状态（如取件成功）。
入口参数：new_state-目标状态；quantity-需要播报的数量值。
返回  值：无。
**************************************************************************/
void app_voice_set_state_with_quantity(system_state_t new_state, uint8_t quantity)
{
    if (xSemaphoreTake(voice_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (voice_manager.current_state != new_state) {
            voice_manager.previous_state = voice_manager.current_state;
            voice_manager.current_state = new_state;
            // 重置播放计数和时间，因为状态改变了
            voice_manager.state_play_count = 0;
            voice_manager.last_play_time = 0;
        }
        voice_manager.pickup_quantity = quantity;
        xSemaphoreGive(voice_manager.state_mutex);
    }
}

/**************************************************************************
Function: app_voice_get_current_state
Input   : none
Output  : system_state_t
函数功能：通过互斥锁保护，安全地获取当前的语音管理系统状态。
入口参数：无。
返回  值：当前生效的状态枚举值。
**************************************************************************/
system_state_t app_voice_get_current_state(void)
{
    system_state_t current_state = STATE_NORMAL;
    if (xSemaphoreTake(voice_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        current_state = voice_manager.current_state;
        xSemaphoreGive(voice_manager.state_mutex);
    }
    return current_state;
}


/**************************************************************************
Function: app_voice_content_process
Input   : none
Output  : none
函数功能：语音消息中心（生产者）。非阻塞地监听扫码队列和CAN通讯队列，将外部硬事件转化为内部语音状态。
入口参数：无。
返回  值：无。
**************************************************************************/
void app_voice_content_process(void)
{
    slave_response_event_t voice_event;
    scan_event_t scan_evt;
    
    // 处理扫描事件
    if (xVoiceQueue != NULL && 
        xQueueReceive(xVoiceQueue, &scan_evt, 0) == pdTRUE) {
        if (scan_evt.is_valid_order) {
            //app_voice_set_state(STATE_PROCESSING);
            //订单有效时，语音不做响应，显示即可，防止语音播报混乱
            DBG_INFO("Voice:QR_order is valid. Processing... Please wait.\r\n");
        } else {
            app_voice_set_state(STATE_INVALID_QR);
        }
    }

    // 处理CAN事件
    if (xSlaveVoiceQueue != NULL && 
        xQueueReceive(xSlaveVoiceQueue, &voice_event, 0) == pdTRUE) {
        switch (voice_event.event_type) {
            // 取件成功
            case SLAVE_EVENT_ORDER_VALID:
                app_voice_set_state_with_quantity(STATE_PICKUP_SUCCESS, voice_event.quantity);
                break;
						
            // 已被取件
            case SLAVE_EVENT_ORDER_ALREADY_PICKED:
                app_voice_set_state(STATE_ALREADY_PICKED);
                break;
						
            // 无效订单
            case SLAVE_EVENT_ORDER_INVALID:
                app_voice_set_state(STATE_INVALID_ORDER);																																		
                break;
						
            // 通信异常 重新扫码
            case SLAVE_EVENT_ORDER_TIMEOUT:
                    app_voice_set_state(STATE_RESCAN);
                    break;
            
            // 装置未设通道
            case SLAVE_EVENT_DEVICE_NO_CHANNEL:
                    app_voice_set_state(STATE_DEVICE_NO_CHANNEL);
                    break;

            
            default:
                app_voice_set_state(STATE_NORMAL);
                break;
						//此处添加通信异常或超时、重新扫码状态
        }
    }
}


/**************************************************************************
Function: app_voice_play_by_state
Input   : none
Output  : none
函数功能：核心执行任务（消费者）。处理状态机的冷却机制、超时重置机制，并最终驱动硬件层发送语音指令。
入口参数：无。
返回  值：无。
**************************************************************************/
void app_voice_play_by_state(void)
{
    // 静态状态变量
    static system_state_t last_handled_state = STATE_INVALID_ORDER;
    static TickType_t last_voice_time = 0;          
    static uint8_t repeat_count = 0;      
    
    
		system_state_t current_state = app_voice_get_current_state();			// 获取当前的系统状态（如：正常、取餐成功、取餐失败等）
		TickType_t current_time = xTaskGetTickCount();										// 获取当前的系统运行时间（FreeRTOS系统滴答计数）
		// 计算距离上次播报过去了多少毫秒
		// (当前时间 - 上次播报时间) × 每个滴答的毫秒数 = 经过的毫秒数
		TickType_t elapsed_time_ms = (current_time - last_voice_time) * portTICK_PERIOD_MS;
		bool state_changed = (current_state != last_handled_state);				// 判断状态是否发生变化（当前状态 ≠ 上次处理的状态）

		// 状态更新与初始化
    if (state_changed) {
        last_handled_state = current_state;
        repeat_count = 0;   // 状态发生变化，重置播报次数
    }

    // 正常待机状态不需要发任何语音，直接返回
    if (current_state == STATE_NORMAL) {
        return; 
    }

     // 超时重置逻辑（冷却机制）
    if (!state_changed) {
        // 如果已经播完最大次数，并且又过去了一个间隔时间，自动重置状态
        if (repeat_count >= VOICE_MAX_REPEAT_COUNT && elapsed_time_ms >= VOICE_REPEAT_INTERVAL_MS) {
            
            last_handled_state = STATE_NORMAL;
            
            // 调用设置状态函数，将系统的全局状态也切回 NORMAL
            app_voice_set_state(STATE_NORMAL); 
            
            return; 
        }
    }

    // 决策 执行
    if (app_voice_should_repeat_state(repeat_count, elapsed_time_ms)) {
        
        bool play_success = true;

				// 带数量的播报
        if (current_state == STATE_PICKUP_SUCCESS) {
            uint8_t quantity = 0;
            if (xSemaphoreTake(voice_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                quantity = voice_manager.pickup_quantity;
                xSemaphoreGive(voice_manager.state_mutex);
            }
            
            if (quantity > 0) {
                bsp_voice_send_byte(STATE_PICKUP_SUCCESS, quantity);
            } else {
                DBG_WARN("订单餐品数量不大于 0\r\n");
                play_success = false; // 数据异常，不下发指令
            }
        } else {
            // 其他状态正常下发
            bsp_voice_send_cmd(current_state);
        }
        
        // 只有真正发出了指令，才更新时间戳和计数器
        if (play_success) {
            last_voice_time = current_time;
            repeat_count++;
        }
    }
}

/**************************************************************************
Function: display task
Input   : none
Output  : none
函数功能：显示屏显示任务
入口参数：无
返回  值：无
**************************************************************************/
void voice_task(void *pvParameters)
{
    u32 lastWakeTime = getSysTickCnt();
    app_voice_manager_init();
    
    while (1) {
        vTaskDelayUntil(&lastWakeTime, F2T(RATE_10_HZ));
        
        // 处理消息队列
        app_voice_content_process();
        // 播放语音
        app_voice_play_by_state();
    }
}	
