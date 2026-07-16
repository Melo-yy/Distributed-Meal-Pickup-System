#include "app_show.h"
#include "debug_config.h"
#include <stdbool.h>

extern uint8_t OK_4G_flag;

// 全局变量用于存储超时时间（毫秒）
static uint32_t g_state_auto_timeout_ms = STATE_AUTO_TIMEOUT_MS;

static display_manager_t display_manager;

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
函数功能：带互斥锁的显示状态设置，自动保存旧状态并记录变更时刻，串口输出状态描述
入口参数：new_state - 待切换的新状态
返回  值：无
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

/**************************************************************************
Function: app_show_set_state_with_quantity
Input   : new_state, quantity
Output  : none
函数功能：设置带份数信息的显示状态。更新状态、记录切换时间并存储餐品数量
入口参数：new_state-目标状态枚举，
					quantity-待显示的份数
返回  值：无
**************************************************************************/
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
Function: app_show_should_timeout_state
Input   : state
Output  : bool
函数功能：判断指定状态是否属于“临时状态”，即是否需要在显示一段时间后自动返回主界面
入口参数：state-待检测的状态
返回  值：true-需要超时返回，
					false-长期维持（如主界面）
**************************************************************************/
static bool app_show_should_timeout_state(system_state_t state)
{
    // 定义需要自动超时返回普通状态的列表
    // 一般来说，除了 STATE_NORMAL 和一些长期状态外，其他状态都应该超时返回
    switch(state) {
        case STATE_NORMAL:
            return false;  // 正常状态不需要超时
        case STATE_PICKUP_SUCCESS:
            return true;   // 取餐成功状态需要超时返回
        case STATE_PROCESSING:
            return true;   // 处理中状态需要超时返回
        case STATE_INVALID_QR:
            return true;   // 无效二维码状态需要超时返回
        case STATE_INVALID_ORDER:
            return true;   // 无效订单状态需要超时返回
        case STATE_ALREADY_PICKED:
            return true;   // 已被取餐状态需要超时返回
        default:
            // 其他状态（如各种错误状态）也都需要超时返回
            return true;
    }
}


/**************************************************************************
Function: app_show_set_timeout_ms
Input   : timeout_ms
Output  : none
函数功能：动态设置显示状态的自动超时切换时间
入口参数：timeout_ms-超时毫秒数
返回  值：无
**************************************************************************/
void app_show_set_timeout_ms(uint32_t timeout_ms)
{
    if (timeout_ms > 0) {
        g_state_auto_timeout_ms = timeout_ms;
        DBG_INFO("DISPLAY: 超时时间已设置为 %u ms\n", timeout_ms);
    }
}


/**************************************************************************
Function: app_show_update_content
Input   : none
Output  : none
函数功能：显示模块核心任务。处理状态超时监测、份数变化监测及最终串口屏指令下发
入口参数：无
返回  值：无
**************************************************************************/
void app_show_update_content(void)
{
    static system_state_t last_displayed_state = STATE_INVALID_ORDER;  // 初始化为无效值
    static uint8_t last_displayed_quantity = 0;
    static TickType_t timeout_start_tick = 0;  // 用于记录状态改变时间
    static bool timeout_enabled = false;       // 标记当前状态是否需要超时返回普通状态
    
    system_state_t current_state = app_show_get_current_state();
    uint8_t current_quantity = 0;
    
    // 获取当前份数
    if (xSemaphoreTake(display_manager.state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        current_quantity = display_manager.pickup_quantity;
        xSemaphoreGive(display_manager.state_mutex);
    }
    
    // 检查是否需要从当前状态超时返回普通状态
    if (timeout_enabled) {
        TickType_t current_tick = xTaskGetTickCount();
        TickType_t elapsed_ticks = current_tick - timeout_start_tick;
        TickType_t timeout_ticks = pdMS_TO_TICKS(g_state_auto_timeout_ms); // 状态自动超时返回普通状态
        
        if (elapsed_ticks >= timeout_ticks) {
            // 检查当前状态是否需要超时返回普通状态
            if (app_show_should_timeout_state(current_state)) {
                app_show_set_state(STATE_NORMAL);
                timeout_enabled = false;
                DBG_INFO("DISPLAY: 状态 %d 自动超时返回 NORMAL\n", current_state);
            }
        }
    }
    
    // 检查状态是否发生变化，如果是，则更新超时标记
    if (current_state != last_displayed_state) {
        // 对于需要超时返回普通状态的状态，启用超时
        if (app_show_should_timeout_state(current_state)) {
            timeout_enabled = true;
            timeout_start_tick = xTaskGetTickCount();
        } else {
            timeout_enabled = false;
        }
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

/**************************************************************************
Function: process_host_can_display_event
Input   : event
Output  : none
函数功能：解析并处理来自主机 CAN 协议的显示事件（如取件结果、订单校验结果）
入口参数：event-指向主机响应事件结构体的常量指针
返回  值：无
**************************************************************************/
static void process_host_can_display_event(const Host_response_event_t *event)
{
    switch (event->event_type) {
        // 有效订单，直接显示取件成功
        case ORDER_EVENT_VALID:
            app_show_set_state_with_quantity(STATE_PICKUP_SUCCESS, event->quantity);
            break;
                
        // 已被取件		
        case ORDER_EVENT_ALREADY_PICKED:
            app_show_set_state(STATE_ALREADY_PICKED);
            break;
                
        // 无效订单或者通道错误（已经通过装置ID和PK_ID得到商品ID，但结合订单ID查询不到）
        case ORDER_EVENT_INVALID:
            app_show_set_state(STATE_INVALID_ORDER);
            break;
        
        // 超时重新扫码
        case ORDER_EVENT_TIMEOUT:
            app_show_set_state(STATE_RESCAN);
            break;
        
        // 装置未设置通道
        case ORDER_EVENT_DEVICE_NO_CHANNEL:
            app_show_set_state(STATE_DEVICE_NO_CHANNEL);
            break;

        // 其它情况显示正常工作
        default:
            app_show_set_state(STATE_NORMAL);
            break;
    }
}

/**************************************************************************
Function: process_host_scan_display_event
Input   : event
Output  : none
函数功能：解析并处理来自扫码模块的显示事件（如开始处理、二维码无效）
入口参数：event-指向扫码事件结构体的常量指针
返回  值：无
**************************************************************************/
static void process_host_scan_display_event(const scan_event_t *event)
{
    if(event->is_valid_order){
        //订单有效，显示处理中
        app_show_set_state(STATE_PROCESSING);
    }else{
        // 无效二维码
        app_show_set_state(STATE_INVALID_QR);
    }
}

/**************************************************************************
Function: process_4G_display_event
Input   : event
Output  : none
函数功能：解析并处理来自 4G 网络模块的显示事件（如数据下载结果、云端错误）
入口参数：event-指向 4G 流程事件结构体的常量指针
返回  值：无
**************************************************************************/
static void process_4G_display_event(const order_proc_event_t *event)
{
    switch (event->state_4G) {

        // 本地更新完成
        case ORDER_PROC_LOCAL_UPDATE_SUCCESS:
            app_show_set_state(STATE_LOCAL_UPDATE_DONE);
            break;
            
        // 本地更新失败
        case ORDER_PROC_LOCAL_UPDATE_FAIL:
            app_show_set_state(STATE_LOCAL_UPDATE_FAIL);
            break;    
            
        // 派送地点为空
        case ORDER_PROC_PLACE_NULL:
            app_show_set_state(STATE_DEVICE_NO_DISPATCH_POINT);
            break;
            
        // 订单已全部取完(无待派送数据)
        case ORDER_PROC_ALL_PICKED:
            app_show_set_state(STATE_NO_WAITING_DISPATCH_DATA);
            break;

        // 通道状态无效（派送点未设通道）
        case ORDER_PROC_CHANNEL_INVALID:
            app_show_set_state(STATE_CHANNEL_NOT_SET);
            break;   

        // 派送-订单数据下载成功
        case ORDER_PROC_ORDER_DOWNLOAD_SUCCESS:
            app_show_set_state(STATE_DISPATCH_UPDATE_DONE);
            break;

        // 派送-订单数据下载失败
        case ORDER_PROC_ORDER_DOWNLOAD_FAIL:
            app_show_set_state(STATE_DOWNLOAD_FAIL);
            break;

        // 云端时间请求失败
        case ORDER_PROC_CLOUD_TIME_FAIL:
        // 严重错误
        case ORDER_PROC_FATAL_ERROR:
        // 其他未知错误
        case ORDER_PROC_UNKNOWN_STATE:
            app_show_set_state(STATE_CLOUD_ERROR);
            break;
            
        // 默认情况
        default:
            app_show_set_state(STATE_NORMAL);
            break;
    }
}

/**************************************************************************
Function: app_show_content_show
Input   : none
Output  : none
函数功能：显示事件接收中心。非阻塞轮询扫码、CAN、4G 三个队列并分发处理
入口参数：无
返回  值：无
**************************************************************************/
void app_show_content_show(void)
{  
		Host_response_event_t display_event;
		scan_event_t show_event;
		order_proc_event_t recv_evt_show;
		// 等待二维码扫描模块传来显示事件（非阻塞方式）
		if (xDisplayQueue != NULL && 
				xQueueReceive(xDisplayQueue, &show_event, 0) == pdTRUE) {			
						process_host_scan_display_event(&show_event);
						vTaskDelay(pdMS_TO_TICKS(50));
		}	

		// 等待主机CAN传来显示事件（非阻塞方式）
		if (xHostDisplayQueue != NULL && 
				xQueueReceive(xHostDisplayQueue, &display_event, 0) == pdTRUE) {			
						process_host_can_display_event(&display_event);
		}

        // 等待4G查询传来显示事件（非阻塞方式）
		if (xOrderProc4G_Show_Queue != NULL && 
				xQueueReceive(xOrderProc4G_Show_Queue, &recv_evt_show, 0) == pdTRUE) {			
						process_4G_display_event(&recv_evt_show);
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
		 while(1)
		 {	
				vTaskDelayUntil(&lastWakeTime, F2T(RATE_20_HZ));
				
				// 处理消息队列
				app_show_content_show();
			 
				// 处理状态并显示
			  app_show_update_content();

		 }	
}