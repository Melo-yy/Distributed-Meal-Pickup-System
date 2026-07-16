#include "app_voice.h"
#include "debug_config.h"
#include <stdbool.h>

static voice_manager_t voice_manager;

/**************************************************************************
Function: app_voice_should_repeat_state
Input   : repeat_count, elapsed_time_ms
Output  : bool
函数功能：判断当前状态是否满足播放条件（首次播放或满足间隔的重复播放）
入口参数：repeat_count-当前状态已播次数，
					elapsed_time_ms-距离上次播放经过的时间
返回  值：true-允许播放，false-保持沉默
**************************************************************************/
static bool app_voice_should_repeat_state(uint8_t repeat_count, TickType_t elapsed_time_ms)
{
    // 首次播报，默认满足条件
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
函数功能：语音管理模块初始化。清空结构体、设置默认状态并创建互斥锁
入口参数：无
返回  值：无
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
Input   : new_state
Output  : none
函数功能：设置新的语音状态。包含互斥锁保护、状态变更检测及记录变更时间
入口参数：new_state-目标系统状态
返回  值：无
**************************************************************************/
void app_voice_set_state(system_state_t new_state)
{
    if (xSemaphoreTake(voice_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (voice_manager.current_state != new_state) {
            voice_manager.previous_state = voice_manager.current_state;
            voice_manager.current_state = new_state;
            voice_manager.state_change_time = xTaskGetTickCount();
            
            DBG_INFO("VOICE: 状态变更 %d->%d\n", 
                   voice_manager.previous_state, voice_manager.current_state);
        }
        xSemaphoreGive(voice_manager.state_mutex);
    }
}

/**************************************************************************
Function: app_voice_set_state_with_quantity
Input   : new_state, quantity
Output  : none
函数功能：设置带数量信息的语音状态（用于取件成功状态）
入口参数：new_state-目标状态，
					quantity-餐品数量
返回  值：无
**************************************************************************/
void app_voice_set_state_with_quantity(system_state_t new_state, uint8_t quantity)
{
    if (xSemaphoreTake(voice_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (voice_manager.current_state != new_state) {
            voice_manager.previous_state = voice_manager.current_state;
            voice_manager.current_state = new_state;
            voice_manager.state_change_time = xTaskGetTickCount();
        }
        voice_manager.pickup_quantity = quantity;
        xSemaphoreGive(voice_manager.state_mutex);
    }
}

/**************************************************************************
Function: app_voice_get_current_state
Input   : none
Output  : system_state_t
函数功能：线程安全地获取当前语音管理器的系统状态
入口参数：无
返回  值：当前系统状态（system_state_t）
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
函数功能：语音消息中心。从多个队列（扫码、CAN、4G）接收事件并转换为语音状态
入口参数：无
返回  值：无
**************************************************************************/
void app_voice_content_process(void)
{
    Host_response_event_t voice_event;
    scan_event_t scan_evt;
    order_proc_event_t recv_evt_voice;

    // 处理扫描事件
    if (xVoiceQueue != NULL && 
        xQueueReceive(xVoiceQueue, &scan_evt, 0) == pdTRUE) {
        if (scan_evt.is_valid_order) {
            //订单有效时，语音不做响应（不播放处理中），显示即可，防止语音播报混乱
            DBG_DEBUG("Voice:QR_order is valid. Processing... Please wait.\r\n");
        } else {
            app_voice_set_state(STATE_INVALID_QR);
        }
    }

    // 处理CAN事件
    if (xHostVoiceQueue != NULL && 
        xQueueReceive(xHostVoiceQueue, &voice_event, 0) == pdTRUE) {
            switch (voice_event.event_type) {
                // 等待取件
                case ORDER_EVENT_VALID:
                    app_voice_set_state_with_quantity(STATE_PICKUP_SUCCESS, voice_event.quantity);
                    break;
                
                // 已被取件
                case ORDER_EVENT_ALREADY_PICKED:
                    app_voice_set_state(STATE_ALREADY_PICKED);
                    break;
                
                // 无效订单
                case ORDER_EVENT_INVALID:
                    app_voice_set_state(STATE_INVALID_ORDER);
                    break;
                
                // 重新扫码
                case ORDER_EVENT_TIMEOUT:
                    app_voice_set_state(STATE_RESCAN);
                    break;
                
                // 装置未设通道
                case ORDER_EVENT_DEVICE_NO_CHANNEL:
                    app_voice_set_state(STATE_DEVICE_NO_CHANNEL);
                    break;

                default:
                    app_voice_set_state(STATE_NORMAL);
                    break;
            }
    }

    // 等待4G查询传来显示事件（非阻塞方式）
    if (xOrderProc4G_Show_Queue != NULL && 
        xQueueReceive(xOrderProc4G_Show_Queue, &recv_evt_voice, 0) == pdTRUE) {			
            switch (recv_evt_voice.state) {
                // 本地更新完成
                case ORDER_PROC_LOCAL_UPDATE_SUCCESS:
                    app_voice_set_state(STATE_LOCAL_UPDATE_DONE);
                    break;
                    
                // 本地更新失败
                case ORDER_PROC_LOCAL_UPDATE_FAIL:
                    app_voice_set_state(STATE_LOCAL_UPDATE_FAIL);
                    break;    
                    
                // 派送地点为空
                case ORDER_PROC_PLACE_NULL:
                    app_voice_set_state(STATE_DEVICE_NO_DISPATCH_POINT);
                    break;
                    
                // 订单已全部取完(无待派送数据)
                case ORDER_PROC_ALL_PICKED:
                    app_voice_set_state(STATE_NO_WAITING_DISPATCH_DATA);
                    break;

                // 通道状态无效（派送点未设通道）
                case ORDER_PROC_CHANNEL_INVALID:
                    app_voice_set_state(STATE_CHANNEL_NOT_SET);
                    break;   

                // 派送-订单数据下载成功
                case ORDER_PROC_ORDER_DOWNLOAD_SUCCESS:
                    app_voice_set_state(STATE_DISPATCH_UPDATE_DONE);
                    break;

                // 派送-订单数据下载失败
                case ORDER_PROC_ORDER_DOWNLOAD_FAIL:
                    app_voice_set_state(STATE_DOWNLOAD_FAIL);
                    break;

                // 云端时间请求失败
                case ORDER_PROC_CLOUD_TIME_FAIL:
                // 严重错误
                case ORDER_PROC_FATAL_ERROR:
                // 其他未知错误
                case ORDER_PROC_UNKNOWN_STATE:
                    app_voice_set_state(STATE_CLOUD_ERROR);
                    break;
                    
                // 默认情况
                default:
                    app_voice_set_state(STATE_NORMAL);
                    break;
            }
        }
}

/**************************************************************************
Function: app_voice_play_by_state
Input   : none
Output  : none
函数功能：语音播放核心任务逻辑。处理状态切换、重复播报计数及最终驱动指令下发
入口参数：无
返回  值：无
**************************************************************************/
void app_voice_play_by_state(void)
{
    // 静态状态变量
    static system_state_t last_handled_state = STATE_INVALID_ORDER;
    static TickType_t last_voice_time = 0;          
    static uint8_t repeat_count = 0;      
    
    system_state_t current_state = app_voice_get_current_state();
    TickType_t current_time = xTaskGetTickCount();
    TickType_t elapsed_time_ms = (current_time - last_voice_time) * portTICK_PERIOD_MS;
    
    bool state_changed = (current_state != last_handled_state);

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
        vTaskDelayUntil(&lastWakeTime, F2T(RATE_20_HZ));
        
        // 处理消息队列
        app_voice_content_process();
        // 播放语音
        app_voice_play_by_state();

    }
}
	
  

