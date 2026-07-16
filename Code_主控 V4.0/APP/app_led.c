#include "app_led.h"


// 4G状态配置
static const led_config_t led_4g_configs[LED_4G_STATE_COUNT] = {
    {0, 0, 1, 0, 0,    "4G unreachable"},     	 // LED_4G_DISCONNECTED
    {0, 0, 1, 1, 500,  "4G connecting"},      	 // LED_4G_CONNECTING
    {0, 1, 0, 0, 0,    "4G ready"},      		 // LED_4G_NORMAL
    {0, 1, 0, 1, 500,  "4G data traffic"}        // LED_4G_DATA_TRANSFER
};



// 当前状态
static led_4g_state_t current_4g_state = LED_4G_DISCONNECTED;

/**************************************************************************
Function: LED初始化
Input   : none
Output  : none
**************************************************************************/
void app_LED_Init(void)
{
    // 初始化BSP层
    bsp_LED_Host_Init();
    
    // 设置初始状态
    current_4g_state = LED_4G_DISCONNECTED;
    
    // 应用初始状态
    app_LED_Update();
    
    DBG_INFO("APP LED: Initialized completely\r\n");
    DBG_INFO("APP LED: 4G State - %s\r\n", led_4g_configs[current_4g_state].description);

}

/**************************************************************************
Function: 更新LED显示状态
Input   : none
Output  : none
**************************************************************************/
void app_LED_Update(void)
{
    // 更新4G LED
    const led_config_t* g4g_config = &led_4g_configs[current_4g_state];
    if (g4g_config->is_blink) {
        bsp_LED_Host_SetBlinkColor(HOST_LED_4G, g4g_config->red, g4g_config->green, g4g_config->blue, g4g_config->blink_interval);
    } else {
        bsp_LED_Host_SetSolidColor(HOST_LED_4G, g4g_config->red, g4g_config->green, g4g_config->blue);
    }
    

}

/**************************************************************************
Function: 设置4G LED状态
Input   : state - 4G状态
Output  : none
**************************************************************************/
void LED_Set4GState(led_4g_state_t state)
{
    if (state >= LED_4G_STATE_COUNT) {
        DBG_ERROR("LED Error: Invalid 4G state %d\r\n", state);
        return;
    }
    
    if (current_4g_state != state) {
        current_4g_state = state;
        DBG_INFO("4G State: %s\r\n", led_4g_configs[state].description);
        app_LED_Update();
    }
}

/**************************************************************************
Function: 设置数据LED状态
Input   : state - 数据状态
Output  : none
**************************************************************************/


/**************************************************************************
Function: 获取4G状态描述
Input   : none
Output  : 状态描述字符串
**************************************************************************/
const char* LED_Get4GStateDesc(void)
{
    return led_4g_configs[current_4g_state].description;
}

/**************************************************************************
Function: 获取数据状态描述
Input   : none
Output  : 状态描述字符串
**************************************************************************/


// 根据4G模块发送的来调整LED
void app_led_4Gstatus_callback(void)
{
    order_proc_event_t recv_evt_led;
    // 等待主机CAN传来显示事件（非阻塞方式）
    if (xOrderProc4G_LED_Queue != NULL && 
            xQueueReceive(xOrderProc4G_LED_Queue, &recv_evt_led, 0) == pdTRUE) {		
                // 先处理4G通信状态（优先级高）
                switch (recv_evt_led.state_4G) {
                    case STATUS_4G_DISCONNECTED:     // 无法通信
                        LED_Set4GState(LED_4G_DISCONNECTED);
                        break;
                        
                    case STATUS_4G_CONNECTING:       // 尝试连接
                        LED_Set4GState(LED_4G_CONNECTING);
                        break;
                        
                    case STATUS_4G_NORMAL:           // 通信正常
                        LED_Set4GState(LED_4G_NORMAL);
                        break;
                        
                    case STATUS_4G_TRANSFER:         // 数据传输中
                        LED_Set4GState(LED_4G_DATA_TRANSFER);
                        break;
                        
                    default:
                        // 未知的4G状态，使用默认
                        LED_Set4GState(LED_4G_NORMAL);
                        break;
                }

                DBG_DEBUG("LED Status Update:  state_4G=%d\n", recv_evt_led.state_4G);
            }


}


/**************************************************************************
Function: LED灯任务
Input   : none
Output  : none
**************************************************************************/
void led_task(void *pvParameters)
{
    app_LED_Init();
    u32 lastWakeTime = getSysTickCnt();
    
    while(1)
    {
        vTaskDelayUntil(&lastWakeTime, F2T(RATE_10_HZ)); // 100ms
        
        app_led_4Gstatus_callback();
        
        // 更新BSP层闪烁状态（统一LED控制）
        bsp_LED_Host_UpdateBlink();
    }
}



