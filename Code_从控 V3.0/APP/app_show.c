#include "app_show.h"

// 全局变量定义
static display_manager_t display_manager;
static uint32_t g_state_auto_timeout_ms = APP_SHOW_STATE_AUTO_TIMEOUT_MS_DEFAULT;  // 状态自动超时时间（毫秒）

// 状态描述映射(英文)  作为调试各个状态输出
static const char* state_descriptions[] = {
    [STATE_NORMAL] = 									"Normal Working",
    [STATE_CLOUD_ERROR] = 						"Abnormal cloud communication",
    [STATE_LOCAL_UPDATE_FAIL] = 			"Local order data Status update failed!",
    [STATE_LOCAL_UPDATE_DONE] = 			"The status of the local order data has been updated successfully!",
    [STATE_DEVICE_NO_DISPATCH_POINT] = "The device does not have a delivery point set up.",
    [STATE_NO_WAITING_DISPATCH_DATA] = "No delivery data required!",
    [STATE_CHANNEL_NOT_SET] = 				"Delivery point does not have a channel.",
    [STATE_DOWNLOAD_FAIL] = 					"Data delivery for download failed ! ! !",
    [STATE_DISPATCH_UPDATE_DONE] = 		"Data delivery update has been completed !",
	  [STATE_DEVICE_NO_CHANNEL] = 			"The device does not have a channel.",
    [STATE_RESCAN] = 									"Communication error. Please scan the code again.",
    [STATE_PROCESSING] = 							"Processing... Please wait.",
    [STATE_PICKUP_SUCCESS] = 					"Pickup successful",
    [STATE_INVALID_QR] = 							"Invalid QR code",
    [STATE_ALREADY_PICKED] = 					"The order has been picked up.",
    [STATE_INVALID_ORDER] = 					"Order invalid"
};


/**************************************************************************
Function: Display manager initialization
Input   : none
Output  : none
函数功能：显示管理器初始化
入口参数：无
返回  值：无
初始化显示状态、前后状态缓存、状态变更时间戳及互斥锁
**************************************************************************/
void app_show_manager_init(void)
{
    memset(&display_manager, 0, sizeof(display_manager));
    display_manager.current_state = STATE_NORMAL;
    display_manager.previous_state = STATE_NORMAL;
		display_manager.pickup_quantity = 0;  
	
    // 创建互斥锁
    display_manager.state_mutex = xSemaphoreCreateMutex();
    if (display_manager.state_mutex == NULL) {
        DBG_ERROR("DISPLAY: Failed to create mutex!\n");
    }
}


/**************************************************************************
Function: Set display state with mutex protection
Input   : new_state - new display state to be applied
Output  : none
函数功能：带互斥锁的显示状态设置
入口参数：new_state - 待切换的新状态
返回  值：无
自动保存旧状态并记录变更时刻，串口输出状态描述
**************************************************************************/
void app_show_set_state(system_state_t new_state)
{
    // 获取互斥锁（等待10ms）
    if (xSemaphoreTake(display_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        
        if (display_manager.current_state != new_state) {
            display_manager.previous_state = display_manager.current_state;
            display_manager.current_state = new_state;
            display_manager.state_change_time = xTaskGetTickCount();
            
            DBG_INFO("DISPLAY: 状态变更 %d->%d: %s\n", 
                   display_manager.previous_state,
                   display_manager.current_state,
                   app_show_get_state_description(new_state));
        }
        
        // 释放互斥锁
        xSemaphoreGive(display_manager.state_mutex);
    } else {
        DBG_ERROR("DISPLAY: 获取互斥锁失败\n");
    }
}

// 修改状态设置函数，支持存储份数
void app_show_set_state_with_quantity(system_state_t new_state, uint8_t quantity)
{
    if (xSemaphoreTake(display_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        
        if (display_manager.current_state != new_state) {
            display_manager.previous_state = display_manager.current_state;
            display_manager.current_state = new_state;
            display_manager.state_change_time = xTaskGetTickCount();
            
            DBG_INFO("DISPLAY: 状态变更 %d->%d: %s\n", 
                   display_manager.previous_state,
                   display_manager.current_state,
                   app_show_get_state_description(new_state));
        }
        
        // 存储份数
        display_manager.pickup_quantity = quantity;
        
        xSemaphoreGive(display_manager.state_mutex);
    }
}

/**************************************************************************
Function: Get current display state with mutex protection
Input   : none
Output  : system_state_t - current display state
函数功能：获取当前显示状态
入口参数：无
返回  值：当前显示状态枚举值；若取锁失败则返回默认NORMAL
**************************************************************************/
system_state_t app_show_get_current_state(void)
{
    system_state_t current_state = STATE_NORMAL;
    
    if (xSemaphoreTake(display_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        current_state = display_manager.current_state;
        xSemaphoreGive(display_manager.state_mutex);
    }
    
    return current_state;
}

/**************************************************************************
Function: Get description string of a given display state
Input   : state - display state enumeration value
Output  : const char* - pointer to static description string
函数功能：根据状态值返回对应描述字符串
入口参数：state - 状态枚举值
返回  值：状态描述字符串指针；若枚举越界返回"Unknow Error"
*************************************************************************/
const char* app_show_get_state_description(system_state_t state)
{
    if (state <= STATE_INVALID_ORDER) {
        return state_descriptions[state];
    }
    return "Unknow Error";
}

/**************************************************************************
Function: Update display content according to current state
Input   : state - display state enumeration value
Output  : none
函数功能：根据当前显示状态刷新屏幕内容，依据状态码调用对应函数，完成界面切换
入口参数：state - 当前状态枚举值
返回  值：无
**************************************************************************/
void app_show_update_content(void)
{
    static system_state_t last_displayed_state = STATE_INVALID_ORDER;  // 初始化为无效值
    static uint8_t last_displayed_quantity = 0;
    
    system_state_t current_state = app_show_get_current_state();
    uint8_t current_quantity = 0;
    
    // 获取当前份数
    if (xSemaphoreTake(display_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        current_quantity = display_manager.pickup_quantity;
        xSemaphoreGive(display_manager.state_mutex);
    }
    
    // 只有当状态或份数发生变化时才刷新
    if (current_state != last_displayed_state || 
        (current_state == STATE_PICKUP_SUCCESS && current_quantity != last_displayed_quantity)) {
        
        if (current_state >= STATE_NORMAL && current_state <= STATE_INVALID_ORDER) {
            bsp_show_switch("page %d\xff\xff\xff", current_state);

            
            if(current_state == STATE_PICKUP_SUCCESS){
                vTaskDelay(pdMS_TO_TICKS(50));
                if (current_quantity > 0) {
                    bsp_show_switch("n0.val=%d\xff\xff\xff", current_quantity);
                }
            }
        }
        
        // 更新最后显示的状态和份数
        last_displayed_state = current_state;
        last_displayed_quantity = current_quantity;
    }
}

// 统一的从机显示事件处理
static void process_host_can_display_event(const slave_response_event_t *event)
{
    switch (event->event_type) {
		// 有效订单，直接显示取件成功
		case SLAVE_EVENT_ORDER_VALID:
				app_show_set_state_with_quantity(STATE_PICKUP_SUCCESS, event->quantity);
		break;
				
		// 已被取件		
		case SLAVE_EVENT_ORDER_ALREADY_PICKED:
				app_show_set_state(STATE_ALREADY_PICKED);
		break;
				
		// 无效订单或者通道错误（已经通过装置ID和PK_ID得到商品ID，但结合订单ID查询不到）
		case SLAVE_EVENT_ORDER_INVALID:
				app_show_set_state(STATE_INVALID_ORDER);
		break;
		
		// 通信异常，重新扫码
		case SLAVE_EVENT_ORDER_TIMEOUT:
				app_show_set_state(STATE_RESCAN);
		break;
		
            // 装置未设置通道
            case SLAVE_EVENT_DEVICE_NO_CHANNEL:
                app_show_set_state(STATE_DEVICE_NO_CHANNEL);
            break;
				
		// 其它情况显示正常工作
		default:
				app_show_set_state(STATE_NORMAL);
		break;
    }
}


// 统一的主机显示事件处理
static void process_host_scan_display_event(const scan_event_t *event)
{
	//订单有效，显示处理中
	if(event->is_valid_order){
			app_show_set_state(STATE_PROCESSING);
	}else{
			app_show_set_state(STATE_INVALID_QR);
	}
}

void app_show_content_show(void)
{  
		slave_response_event_t display_event;
		scan_event_t show_event;
		
			// 等待二维码扫描模块传来显示事件（非阻塞方式）
		if (xDisplayQueue != NULL && 
				xQueueReceive(xDisplayQueue, &show_event, 0) == pdTRUE) {			
						process_host_scan_display_event(&show_event);
						vTaskDelay(pdMS_TO_TICKS(50));
		}	

		// 等待从机CAN传来显示事件（非阻塞方式）
		if (xSlaveDisplayQueue != NULL && 
				xQueueReceive(xSlaveDisplayQueue, &display_event, 0) == pdTRUE) {			
						process_host_can_display_event(&display_event);
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
void show_task(void *pvParameters)
{
		u32 lastWakeTime = getSysTickCnt();
		app_show_manager_init();
		DBG_INFO("show task begin!\r\n\r\n");
		 while(1)
		 {	
				vTaskDelayUntil(&lastWakeTime, F2T(RATE_10_HZ));
				
				// 处理消息队列
				app_show_content_show();
			 
				// 处理状态并显示
			  app_show_update_content();

        // 检查是否需要自动超时返回STATE_NORMAL状态
        if (xSemaphoreTake(display_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            system_state_t current_state = display_manager.current_state;
            uint32_t current_time = xTaskGetTickCount();
            uint32_t time_diff_ms = (current_time - display_manager.state_change_time) * portTICK_PERIOD_MS;

            if (current_state != STATE_NORMAL && app_show_should_timeout_state(current_state)) {
                if (time_diff_ms >= g_state_auto_timeout_ms) {
                    display_manager.previous_state = display_manager.current_state;
                    display_manager.current_state = STATE_NORMAL;
                    display_manager.state_change_time = current_time;
                    
                    DBG_INFO("DISPLAY: 状态自动超时 %d->%d: %s (超时时间: %lu ms)\n", 
                           display_manager.previous_state,
                           display_manager.current_state,
                           app_show_get_state_description(STATE_NORMAL),
                           g_state_auto_timeout_ms);
                }
            }
            xSemaphoreGive(display_manager.state_mutex);
        }

		 }	
}

// 状态自动超时相关函数实现

// 判断指定状态是否需要自动超时返回STATE_NORMAL
bool app_show_should_timeout_state(system_state_t state)
{
    switch (state) {
        case STATE_RESCAN:           // 重新扫码
        case STATE_PICKUP_SUCCESS:   // 取件成功
        case STATE_INVALID_QR:       // 无效二维码
        case STATE_ALREADY_PICKED:   // 已被取件
        case STATE_INVALID_ORDER:    // 无效订单
        case STATE_DEVICE_NO_CHANNEL: // 装置未设通道
        case STATE_PROCESSING:       // 处理中
            return true;
        default:
            return false;
    }
}

// 设置状态自动超时时间（毫秒）
void app_show_set_timeout_ms(uint32_t timeout_ms)
{
    g_state_auto_timeout_ms = timeout_ms;
    DBG_INFO("DISPLAY: 状态自动超时时间设置为 %lu ms\n", timeout_ms);
}

// 获取当前状态自动超时时间（毫秒）
uint32_t app_show_get_timeout_ms(void)
{
    return g_state_auto_timeout_ms;
}