#include "app_led.h"

///* 颜色表：红、绿、蓝、黄、紫、青、白、灭 */
//static const uint8_t color_tab[8][3] = {
//    {1,0,0}, {0,1,0}, {0,0,1}, {1,1,0},
//    {1,0,1}, {0,1,1}, {1,1,1}, {0,0,0}
//};

// 从控LED状态配置 - 简化到4个状态
static const slave_led_config_t slave_led_configs[] = {
// 状态 R  G  B 闪烁间隔(ms)   描述
    {0, 1, 0, 0,   0,    "communication normal"},      // LED_SLAVE_STATE_COMM_NORMAL - 绿色常亮
    {0, 1, 0, 1,   300,  "data transferring"},         // LED_SLAVE_STATE_DATA_TRANSFER - 绿色闪烁
    {0, 0, 1, 0,   0,    "communication lost"},        // LED_SLAVE_STATE_NO_COMM - 蓝色常亮
    {0, 0, 1, 1,   300,  "communication lost"}         // LED_SLAVE_STATE_COMM_ATTEMPT - 蓝色闪烁
};

// 当前状态
static led_slave_state_t current_slave_state = LED_SLAVE_STATE_NO_COMM;


/**************************************************************************
Function: 从控LED初始化
Input   : none
Output  : none
**************************************************************************/
void app_LED_Slave_Init(void)
{
    // 设置初始状态 - 无法通信
    current_slave_state = LED_SLAVE_STATE_NO_COMM;
    
    // 应用初始状态
    app_LED_Slave_Update();
    
    DBG_INFO("APP LED Slave: Initialized completely\r\n");
    DBG_INFO("APP LED Slave: Current State - %s\r\n", slave_led_configs[current_slave_state].description);
}

/**************************************************************************
Function: 设置从控LED状态
Input   : state - 从控状态
Output  : none
**************************************************************************/
void LED_Slave_SetState(led_slave_state_t state)
{
    if (state >= LED_SLAVE_STATE_COUNT) {
        DBG_ERROR("LED Slave Error: Invalid state %d\r\n", state);
        return;
    }
    
    if (current_slave_state != state) {
        current_slave_state = state;
				// 使用简短的打印
        switch(state) {
            case LED_SLAVE_STATE_COMM_NORMAL: DBG_INFO("LED: NORMAL\r\n"); break;
            case LED_SLAVE_STATE_DATA_TRANSFER: DBG_INFO("LED: TRANSFER\r\n"); break;
            case LED_SLAVE_STATE_NO_COMM: DBG_INFO("LED: NO_COMM\r\n"); break;
            case LED_SLAVE_STATE_COMM_ATTEMPT: DBG_INFO("LED: ATTEMPT\r\n"); break;
        }       
				app_LED_Slave_Update();
    }
}

/**************************************************************************
Function: 获取从控状态描述
Input   : none
Output  : 状态描述字符串
**************************************************************************/
const char* LED_Slave_GetStateDesc(void)
{
    return slave_led_configs[current_slave_state].description;
}


/**************************************************************************
Function: 更新从控LED显示状态
Input   : none
Output  : none
**************************************************************************/
void app_LED_Slave_Update(void)
{
    const slave_led_config_t* config = &slave_led_configs[current_slave_state];
    
    if (config->is_blink) {
        bsp_LED_Slave_SetBlinkColor(SLAVE_LED_POWER, config->red, config->green, config->blue, config->blink_interval);
    } else {
        bsp_LED_Slave_SetSolidColor(SLAVE_LED_POWER, config->red, config->green, config->blue);
    }
}

/**************************************************************************
Function: 内部处理函数（只在 led_task 里调用）
**************************************************************************/
static void app_LED_Slave_ProcessQueue(void)
{
    slave_led_event_t evt;
   if (xSlaveLEDQueue != NULL && 
            xQueueReceive(xSlaveLEDQueue, &evt, 0) == pdTRUE){

        switch (evt.event_type) {
            case SLAVE_LED_EVENT_COMM_ATTEMPT:
                LED_Slave_SetState(LED_SLAVE_STATE_COMM_ATTEMPT);
                break;

            case SLAVE_LED_EVENT_NO_COMM:
                LED_Slave_SetState(LED_SLAVE_STATE_NO_COMM);
                break;

            case SLAVE_LED_EVENT_DATA_TRANSFER:
                LED_Slave_SetState(LED_SLAVE_STATE_DATA_TRANSFER);
                break;

            case SLAVE_LED_EVENT_COMM_NORMAL:
                LED_Slave_SetState(LED_SLAVE_STATE_COMM_NORMAL);
                break;
        }
    }
}


/**************************************************************************
Function: LED light flashing task
Input   : none
Output  : none
函数功能：LED灯闪烁任务
入口参数：无 
返回  值：无
**************************************************************************/
void led_task(void *pvParameters)
{
    app_LED_Slave_Init();
    TickType_t lastWakeTime = xTaskGetTickCount();
    while(1)
    {
	    vTaskDelayUntil(&lastWakeTime, F2T(RATE_10_HZ)); // 100ms
			
			// 处理从机LED队列
			app_LED_Slave_ProcessQueue();

			// 更新BSP层闪烁状态
      bsp_LED_Slave_UpdateBlink();

    }
}  
