#include "app_can.h"
#include "debug_config.h"

scan_event_t can_event;

// 全局队列定义
QueueHandle_t xSlaveVoiceQueue = NULL;
QueueHandle_t xSlaveDisplayQueue = NULL;
QueueHandle_t xSlaveLEDQueue = NULL;

static slave_connection_t slave_conn = {
    .state = SLAVE_STATE_DISCONNECTED,
    .slave_id = 0,
    .host_id = 0,
    .retry_count = 0,
    .last_ping_time = 0,
    .last_confirm_time = 0,
    .last_reject_time = 0,
    .last_order_send_time = 0,
    .order_retry_count = 0,
    .max_retries = 5,
    .max_order_retries = 3,        // 订单最多重试3次
    .ping_interval = 1000,
    .confirm_timeout = 5000,
    .reject_wait_time = 60000,
    .order_ack_timeout = 2000      // 2秒订单确认超时
};

//---------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------初始化部分------------------------------------------------------//
//---------------------------------------------------------------------------------------------------------------//

/**************************************************************************
Function: Slave application layer initialization
Input   : none
Output  : none
函数功能：从机应用层初始化
入口参数：无
返回  值：无
**************************************************************************/
void app_can_Slave_Init(void)
{
    // 初始化BSP层CAN
    bsp_can_Init();
    
		// 配置从机过滤器：接收与从机id相同的响应
		bsp_can_Filter_Config(MY_SLAVE_ID, 0x7FF);
    
    DBG_INFO("CAN_Slave: Initialized (ID: 0x%03X)\r\n", MY_SLAVE_ID);
	
}

// 初始化所有队列
void app_can_queues_init(void)
{
    xSlaveVoiceQueue = xQueueCreate(3, sizeof(slave_response_event_t));
    xSlaveDisplayQueue = xQueueCreate(3, sizeof(slave_response_event_t));
    xSlaveLEDQueue      = xQueueCreate(3, sizeof(slave_led_event_t));  
    
    if ( xSlaveVoiceQueue && xSlaveDisplayQueue && xSlaveLEDQueue) {
				DBG_INFO("CAN_TASK: 3 Queues initialized successfully\r\n");
    } else {
        DBG_ERROR("CAN_TASK: Failed to initialize queues\r\n");
    }
}

/**************************************************************************
Function: Initialize slave connection
Input   : slave_id - local slave ID
Output  : none
函数功能：初始化从机连接 
入口参数：slave_id - 本从机ID
返回  值：无
**************************************************************************/
void app_can_Slave_Connection_Init(uint16_t slave_id)
{
    slave_conn.slave_id = slave_id;
    slave_conn.state = SLAVE_STATE_DISCONNECTED;
    slave_conn.host_id = 0;
    slave_conn.retry_count = 0;
    slave_conn.last_ping_time = 0;
    slave_conn.last_confirm_time = 0;
    slave_conn.last_reject_time = 0;
    slave_conn.last_order_send_time = 0;
    slave_conn.order_retry_count = 0;
    memset(slave_conn.pending_order_data, 0, sizeof(slave_conn.pending_order_data));
    slave_conn.pending_order_length = 0;
	
    DBG_INFO("CAN_SLAVE[0x%03X]: Connection initialized\r\n", slave_id);
}


//---------------------------------------------------------------------------------------------------------------//
//---------------------------------------------主机从机连接------------------------------------------------------//
//---------------------------------------------------------------------------------------------------------------//


/**************************************************************************
Function: Slave send ping request to host
Input   : none
Output  : can_status_t status
函数功能：从机向主机发送ping连接请求
入口参数：无
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_SendPingRequest(void)
{
    if (slave_conn.slave_id == 0) {
        return CAN_STATUS_INVALID_ID;
    }
    
    // 准备ping请求数据 [命令类型(1) | 从机ID高字节(1) | 从机ID低字节(1)]
    uint8_t ping_data[3];
    ping_data[0] = CMD_SELF_TEST;
    ping_data[1] = (slave_conn.slave_id >> 8) & 0xFF;   // ID高字节
    ping_data[2] = slave_conn.slave_id & 0xFF;          // ID低字节
    
    // 发送到主机广播ID
    can_status_t status = bsp_can_Transmit(MAKE_RESPONSE_ID(slave_conn.slave_id) , 3, ping_data);
    
    if (status == CAN_STATUS_OK) {
        DBG_INFO("CAN_SLAVE[0x%03X]: Ping request sent to host\r\n", slave_conn.slave_id);
        slave_conn.state = SLAVE_STATE_CONNECTING;
    } else {
        DBG_WARN("CAN_SLAVE[0x%03X]: Ping request send failed: %d\r\n", slave_conn.slave_id, status);
    }
    
    return status;
}

/**************************************************************************
Function: Slave send ping success confirmation to host
Input   : none
Output  : can_status_t status
函数功能：从机向主机发送ping成功确认
入口参数：无
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_SendPingSuccess(void)
{
    if (slave_conn.slave_id == 0 || slave_conn.host_id == 0) {
        return CAN_STATUS_INVALID_ID;
    }
    
    // 准备确认数据 [确认类型(1) | 从机ID高字节(1) | 从机ID低字节(1)]
    uint8_t confirm_data[3];
    confirm_data[0] = PING_SUCCESS;
    confirm_data[1] = (slave_conn.slave_id >> 8) & 0xFF;  // 从机ID高字节
    confirm_data[2] = slave_conn.slave_id & 0xFF;          // 从机ID低字节
    
    // 发送到主机
    can_status_t status = bsp_can_Transmit(MAKE_RESPONSE_ID(slave_conn.slave_id), 3, confirm_data);
    
    if (status == CAN_STATUS_OK) {
        DBG_INFO("CAN_SLAVE[0x%03X]: Ping success confirmation sent to host 0x%03X\r\n", 
               slave_conn.slave_id, slave_conn.host_id);
        slave_conn.last_confirm_time = getSysTickCnt();
    } else {
        DBG_WARN("CAN_SLAVE[0x%03X]: Failed to send ping success confirmation: %d\r\n", 
               slave_conn.slave_id, status);
    }
    
    return status;
}



/**************************************************************************
Function: Process host ping response
Input   : cmd_data - command data pointer
          data_len - data length
Output  : can_status_t status
函数功能：处理主机的ping响应
入口参数：cmd_data - 命令数据指针
          data_len - 数据长度
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_ProcessPingResponse(uint8_t *cmd_data, uint8_t data_len)
{
    // 验证数据长度
    if (data_len + 1  < 3) {
        DBG_ERROR("CAN_SLAVE[0x%03X]: Ping response too short, len=%d\r\n", 
               slave_conn.slave_id, data_len);
        return CAN_STATUS_RESP_ERROR;
    }
    
    // 解析主机ID
    uint8_t host_id_high = cmd_data[0];
    uint8_t host_id_low = cmd_data[1];
    uint16_t received_host_id = (host_id_high << 8) | host_id_low;
    
    // 存储主机ID
    slave_conn.host_id = (uint8_t)received_host_id;
    slave_conn.state = SLAVE_STATE_WAITING_CONFIRM;
    slave_conn.retry_count = 0;
    
    DBG_INFO("CAN_SLAVE[0x%03X]: Received ping response from host 0x%03X\r\n", 
           slave_conn.slave_id, slave_conn.host_id);
    
    // 发送确认信息给主机
    can_status_t status = app_can_Slave_SendPingSuccess();
    
    if (status == CAN_STATUS_OK) {
        slave_conn.state = SLAVE_STATE_CONNECTED;
        DBG_INFO("CAN_SLAVE[0x%03X]: Connection established with host 0x%03X\r\n", 
               slave_conn.slave_id, slave_conn.host_id);
    }
    
		DBG_INFO("status:%d",status);
		
    return status;
} 

/**************************************************************************
Function: Slave connection management with timeout retry
Input   : none
Output  : none
函数功能：从机连接管理（含超时重试机制）
入口参数：无
返回  值：无
**************************************************************************/
void app_can_Slave_ConnectionManager(void)
{
    uint32_t current_time = getSysTickCnt();
    static uint32_t last_connected_print = 0;
	
		static uint32_t last_print = 0;

		// 检查订单响应超时
    app_can_Slave_CheckOrderTimeout();
	
    switch (slave_conn.state) {
        case SLAVE_STATE_DISCONNECTED:
            // 初始状态，开始连接过程
            DBG_INFO("CAN_SLAVE[0x%03X]: Starting connection process\r\n", slave_conn.slave_id);
            slave_conn.state = SLAVE_STATE_CONNECTING;
            slave_conn.retry_count = 0;
						slave_conn.host_id = 0;  // 重置主机ID
            app_can_Slave_SendPingRequest();
            break;
           
				
        case SLAVE_STATE_CONNECTING:
            // 检查是否超时需要重试ping
            if (current_time - slave_conn.last_ping_time > slave_conn.ping_interval) {
                if (slave_conn.retry_count < slave_conn.max_retries) {
                    DBG_WARN("CAN_SLAVE[0x%03X]: Ping timeout, retrying (%d/%d)\r\n", 
                           slave_conn.slave_id, slave_conn.retry_count + 1, slave_conn.max_retries);
									
                    app_can_Slave_SendPingRequest();  // 只负责发，不负责更新时间/次数
									
										// 更新时间 / 次数
										slave_conn.last_ping_time = getSysTickCnt();
										slave_conn.retry_count++;
									
                } else {
                    // 超过最大重试次数
                    DBG_ERROR("CAN_SLAVE[0x%03X]: Connection failed after %d retries\r\n", 
                           slave_conn.slave_id, slave_conn.max_retries);
                    slave_conn.state = SLAVE_STATE_DISCONNECTED;
										slave_conn.retry_count = 0;
                    // 重新尝试
                }
            }
            break;
            
						
        case SLAVE_STATE_WAITING_CONFIRM:
            // 等待主机确认超时处理
            if (current_time - slave_conn.last_confirm_time > slave_conn.confirm_timeout) {
                DBG_WARN("CAN_SLAVE[0x%03X]: Host confirmation timeout, reconnecting\r\n", 
                       slave_conn.slave_id);
                slave_conn.state = SLAVE_STATE_DISCONNECTED;
            }
            break;
            
						
				case SLAVE_STATE_REJECTED:
            // 被主机拒绝状态，等待一段时间后重试
            if (current_time - slave_conn.last_reject_time > slave_conn.reject_wait_time) {
                DBG_INFO("CAN_SLAVE[0x%03X]: Rejection timeout expired, reconnecting...\r\n", 
                       slave_conn.slave_id);
                slave_conn.state = SLAVE_STATE_DISCONNECTED;
            } else {
                // 显示剩余等待时间（每10秒显示一次，避免频繁打印）
                static uint32_t last_reject_print = 0;
                if (current_time - last_reject_print > 10000) {
                    uint32_t remaining_time = (slave_conn.last_reject_time + slave_conn.reject_wait_time - current_time) / 1000;
                    DBG_WARN("CAN_SLAVE[0x%03X]: Rejected by host, waiting %lu seconds before retry\r\n", 
                           slave_conn.slave_id, remaining_time);
                    last_reject_print = current_time;
                }
            }
            break;		
						
				  case SLAVE_STATE_WAITING_ORDER_ACK:
            // 等待主机确认收到订单的状态，检查是否超时
            if (current_time - slave_conn.last_order_send_time > slave_conn.order_ack_timeout) {
                if (slave_conn.order_retry_count < slave_conn.max_order_retries) {
                    DBG_WARN("CAN_SLAVE[0x%03X]: Order ACK timeout, retrying order data (%d/%d)\r\n", 
                           slave_conn.slave_id, slave_conn.order_retry_count + 1, slave_conn.max_order_retries);
                    app_can_Slave_RetryOrderData();
                } else {
                    // 超过最大重试次数
                    DBG_ERROR("CAN_SLAVE[0x%03X]: Order delivery failed after %d retries\r\n", 
                           slave_conn.slave_id, slave_conn.max_order_retries);
                    
                    // 回到已连接状态，但订单发送失败
                    slave_conn.state = SLAVE_STATE_CONNECTED;
                    slave_conn.order_retry_count = 0;
                    memset(slave_conn.pending_order_data, 0, sizeof(slave_conn.pending_order_data));
                    slave_conn.pending_order_length = 0;
                    
                    // 可选：通知用户订单发送失败
                    DBG_WARN("!!! Order delivery to host failed, please try again.\r\n");
                }
            }
            break;		
						
						
				case SLAVE_STATE_CONNECTED:
            // 已连接状态，可以执行其他通信
            // 可以在这里添加心跳检测等维持连接的操作
				
			// 已连接状态，每5秒打印一次连接状态（避免频繁打印）
            if (current_time - last_connected_print > 30000) {
                DBG_INFO("CAN_SLAVE[0x%03X]: Connected to host 0x%03X\r\n", 
                       slave_conn.slave_id, slave_conn.host_id);
                last_connected_print = current_time;
            }
            break;
    }

    // 统一发送LED状态
    app_can_UpdateSlaveLED();
}


/**************************************************************************
Function: Process host rejection
Input   : cmd_data - command data pointer
          data_len - data length
Output  : can_status_t status
函数功能：处理主机拒绝连接
入口参数：cmd_data - 命令数据指针
          data_len - 数据长度
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_ProcessHostRejection(void)
{
    DBG_ERROR("CAN_SLAVE[0x%03X]: Connection rejected by host - This device is not authorized!\r\n", 
           slave_conn.slave_id);
    
    // 进入拒绝状态，设置拒绝时间
    slave_conn.state = SLAVE_STATE_REJECTED;
    app_can_UpdateSlaveLED();      // 蓝色常亮
    slave_conn.last_reject_time = getSysTickCnt();
    slave_conn.retry_count = 0;
    slave_conn.host_id = 0;
    
    // 打印重要警告信息，提醒工作人员
    DBG_ERROR("!!! ATTENTION !!! Slave 0x%03X is not in host's authorized list.\r\n", slave_conn.slave_id);
    DBG_ERROR("!!! Please contact administrator to add this device to the authorized list.\r\n");
    DBG_INFO("!!! This device will retry connection in 60 seconds.\r\n");
    
    return CAN_STATUS_NOT_ALLOWED;
}

/**************************************************************************
Function: app_can_SendSlaveLEDEvent
Input   : event
Output  : none
函数功能：统一发送 LED 状态切换事件。包含状态去重逻辑，防止重复发送相同指令导致的队列拥堵
入口参数：event-LED 事件类型（如常亮、闪烁等颜色模式）
返回  值：无
**************************************************************************/
static void app_can_SendSlaveLEDEvent(slave_led_event_type_t event)
{
    static slave_led_event_type_t last_event = SLAVE_LED_EVENT_COUNT; // 初始化为无效值
    
    if (xSlaveLEDQueue == NULL) return;
    
    // 只发送状态变化的事件
    if (event == last_event) {
        // 相同事件，不重复发送
        return;
    }
    
    // 记录上一次发送的事件
    last_event = event;
    
    slave_led_event_t evt = {
        .event_type = event,
        .slave_id = MY_SLAVE_ID
    };
    
    // 非阻塞发送
    if (xQueueSend(xSlaveLEDQueue, &evt, 0) != pdTRUE) {
        DBG_WARN("xSlaveLEDQueue full! event %d dropped\r\n", event);
    } else {
        DBG_INFO("LED_EVENT_SENT: %d (previous: %d)\r\n", event, last_event);
    }
}

/**************************************************************************
Function: app_can_UpdateSlaveLED
Input   : none
Output  : none
函数功能：根据当前 CAN 链路层状态机（slave_conn.state）自动推导并更新 LED 表现形式
入口参数：无（直接读取全局连接状态变量）
返回  值：无
**************************************************************************/
static void app_can_UpdateSlaveLED(void)
{
    slave_led_event_type_t led_event;

    switch (slave_conn.state)
    {
        case SLAVE_STATE_DISCONNECTED:
        case SLAVE_STATE_REJECTED:
            led_event = SLAVE_LED_EVENT_NO_COMM;          // 蓝色常亮
            break;

        case SLAVE_STATE_CONNECTING:
        case SLAVE_STATE_WAITING_CONFIRM:
            led_event = SLAVE_LED_EVENT_COMM_ATTEMPT;     // 蓝色闪烁
            break;

        case SLAVE_STATE_WAITING_ORDER_ACK:
            led_event = SLAVE_LED_EVENT_DATA_TRANSFER;    // 绿色闪烁
            break;

        case SLAVE_STATE_CONNECTED:
            led_event = SLAVE_LED_EVENT_COMM_NORMAL;      // 绿色常亮
            break;

        default:
            return;  // 未知状态不点灯
    }

    // 直接发事件（你已经封装好的函数）
    app_can_SendSlaveLEDEvent(led_event);
}

//---------------------------------------------------------------------------------------------------------------//
//-------------------------------------------主从机通信及订单处理------------------------------------------------//
//---------------------------------------------------------------------------------------------------------------//

/**************************************************************************
Function: Retry sending pending order data
Input   : none
Output  : can_status_t status
函数功能：重传待确认的订单数据
入口参数：无
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_RetryOrderData(void)
{
    
    can_status_t status = bsp_can_Transmit(MAKE_RESPONSE_ID(slave_conn.slave_id), 
                                          slave_conn.pending_order_length, 
                                          slave_conn.pending_order_data);
    
    if (status == CAN_STATUS_OK) {
        slave_conn.last_order_send_time = getSysTickCnt();
        slave_conn.order_retry_count++;
        
        DBG_INFO("CAN_SLAVE[0x%03X]: Order data retry %d/%d\r\n", 
               slave_conn.slave_id, slave_conn.order_retry_count, slave_conn.max_order_retries);
    } else {
        DBG_WARN("CAN_SLAVE[0x%03X]: Order data retry failed: %d\r\n", 
               slave_conn.slave_id, status);
    }
    
    return status;
}


/**************************************************************************
Function: Process host order acknowledgment
Input   : cmd_data - command data pointer
          data_len - data length
Output  : can_status_t status
函数功能：处理主机订单确认
入口参数：cmd_data - 命令数据指针
          data_len - 数据长度
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_ProcessOrderAck(uint8_t *cmd_data, uint8_t data_len)
{
    // 验证数据长度
    if (data_len < 5) {
        DBG_ERROR("CAN_SLAVE[0x%03X]: Order ACK too short, len=%d\r\n", 
               slave_conn.slave_id, data_len);
        return CAN_STATUS_RESP_ERROR;
    }
    
    // 检查是否在等待订单确认状态
    if (slave_conn.state != SLAVE_STATE_WAITING_ORDER_ACK) {
        DBG_WARN("CAN_SLAVE[0x%03X]: Received order ACK but not waiting for it\r\n", 
               slave_conn.slave_id);
        return CAN_STATUS_INVALID_STATE;
    }
    
    // 解析确认的订单ID（简单验证，可以更复杂:加上订单的验证） 
    uint8_t ack_slave_id = ((cmd_data[0]<< 8) | cmd_data[1]);
    
    if (ack_slave_id != slave_conn.slave_id) {
        DBG_ERROR("CAN_SLAVE[0x%03X]: Order ACK slave ID mismatch, expected:0x%03X, got:0x%03X\r\n", 
               slave_conn.slave_id, slave_conn.slave_id, ack_slave_id);
        return CAN_STATUS_INVALID_ID;
    }
    
    DBG_INFO("CAN_SLAVE[0x%03X]: Order ACK received from host, order delivery confirmed\r\n", 
           slave_conn.slave_id);
    
    // 重置订单发送状态，回到已连接状态
    slave_conn.state = SLAVE_STATE_CONNECTED;
    slave_conn.order_retry_count = 0;
    memset(slave_conn.pending_order_data, 0, sizeof(slave_conn.pending_order_data));
    slave_conn.pending_order_length = 0;
    
    return CAN_STATUS_OK;
}



/**************************************************************************
Function: Check order response timeout
Input   : none
Output  : none
函数功能：检查订单响应超时（主机未回复从机的订单请求）
入口参数：无
返回  值：无
**************************************************************************/
void app_can_Slave_CheckOrderTimeout(void)
{
    static uint32_t last_order_request_time = 0;
    static uint8_t order_pending = 0;
    
    uint32_t current_time = getSysTickCnt();
    
    // 检查是否在等待订单响应状态
    if (slave_conn.state == SLAVE_STATE_WAITING_ORDER_ACK) {
        if (!order_pending) {
            // 开始等待响应
            last_order_request_time = current_time;
            order_pending = 1;
            DBG_INFO("CAN_SLAVE[0x%03X]: Order request sent, waiting for response...\r\n", 
                   slave_conn.slave_id);
        }
        
        // 检查是否超时（10秒）
        if (current_time - last_order_request_time > ORDER_RESPONSE_TIMEOUT) {
            DBG_WARN("CAN_SLAVE[0x%03X]: Order response timeout after %d ms\r\n", 
                   slave_conn.slave_id, ORDER_RESPONSE_TIMEOUT);
            
            // 创建超时事件
            slave_response_event_t timeout_event;
            timeout_event.slave_id = slave_conn.slave_id;
            timeout_event.event_type = SLAVE_EVENT_ORDER_TIMEOUT;
            timeout_event.order_exists = 0;
            timeout_event.channel_num = 0;
            timeout_event.pickup_status = 0;
            timeout_event.quantity = 0;
            memset(timeout_event.order_num, 0, sizeof(timeout_event.order_num));
            
            // 发送到语音和显示队列
            if (xSlaveVoiceQueue != NULL) {
                xQueueSend(xSlaveVoiceQueue, &timeout_event, 0);
            }
            if (xSlaveDisplayQueue != NULL) {
                xQueueSend(xSlaveDisplayQueue, &timeout_event, 0);
            }
            
            // 重置状态
            order_pending = 0;
            slave_conn.state = SLAVE_STATE_CONNECTED;
            slave_conn.order_retry_count = 0;
            
            DBG_INFO("CAN_SLAVE[0x%03X]: Order timeout event generated\r\n", slave_conn.slave_id);
        }
    } else {
        // 不在等待状态，重置标志
        order_pending = 0;
    }
}


/**************************************************************************
Function: Process communication command from host
Input   : slave_id - local slave ID (1-9)
          cmd_data - command data pointer
          data_len - data length
Output  : can_status_t status
函数功能：处理主机通信命令
入口参数：slave_id - 本从机ID(1-9)
          cmd_data - 命令数据指针
          data_len - 数据长度
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_ProcessCommCommand(uint16_t slave_id, uint8_t *cmd_data, uint8_t data_len ,uint8_t *order_num)
{
    if (!IS_VALID_SLAVE_ID(slave_id)) {
        DBG_ERROR("CAN_SLAVE: Invalid slave ID 0x%03X\r\n", slave_id);
        return CAN_STATUS_INVALID_ID;
    }
    
    // 验证命令数据长度（5字节：ID（2字节）+订单是否存在+通道编号+取件状态+数量）
    if (data_len < 6) {
        DBG_ERROR("CAN_SLAVE: Communication command too short, len=%d\r\n", data_len);
        return CAN_STATUS_RESP_ERROR;
    }
    
    // 解析数据 暂存到结构体内
		slave_response_event_t response_event;
    response_event.slave_id = ((cmd_data[0] << 8) | cmd_data[1] );  // 目标从机ID
    response_event.order_exists = cmd_data[2];  										// 订单是否存在
    response_event.channel_num = cmd_data[3];   										// 通道编号
    response_event.pickup_status = cmd_data[4]; 										// 取件状态
    response_event.quantity = cmd_data[5];      										// 数量
    memcpy(response_event.order_num, order_num, 4); 								// 订单ID
		
    // 验证目标ID是否匹配本机
    if (response_event.slave_id != slave_id) {
        DBG_WARN("CAN_SLAVE[0x%03X]: Target ID mismatch, expected:0x%03X\r\n", 
               slave_id, response_event.slave_id);
        return CAN_STATUS_INVALID_ID;
    }
    
    DBG_INFO("CAN_SLAVE[0x%03X]: OrderExists:%d, Channel:%d, Status:%d, Qty:%d\r\n", 
           slave_id, response_event.order_exists, response_event.channel_num, 
           response_event.pickup_status, response_event.quantity);
    
    can_status_t status = CAN_STATUS_OK;
    
    // 根据订单状态设置事件类型
    if (response_event.order_exists) { 
        if (!response_event.pickup_status) { 
            // 订单存在且未取件 - 发送确认响应给主机
            response_event.event_type = SLAVE_EVENT_ORDER_VALID;
            DBG_INFO("CAN_SLAVE[0x%03X]: Valid order, channel %d, quantity %d\r\n", 
                   slave_id, response_event.channel_num, response_event.quantity);
            
						// 重置状态，表示已收到响应
            slave_conn.state = SLAVE_STATE_CONNECTED;
            slave_conn.order_retry_count = 0;
					
            // 准备响应数据（使用结构体中的数据填充）
            uint8_t resp_data[7];
            resp_data[0] = CMD_PICK_OK;                                  			// 响应标识
            resp_data[1] = (response_event.slave_id >> 8) & 0xFF;                   // 从机ID 高字节
            resp_data[2] =  response_event.slave_id  & 0xFF;             			// 从机ID 低字节 
            resp_data[3] =  response_event.order_num[0];        					// 订单编号低字节
            resp_data[4] =  response_event.order_num[1]; 							// 订单编号高字节       
            resp_data[5] =  response_event.order_num[2]; 							// 订单编号高字节       
            resp_data[6] =  response_event.order_num[3]; 							// 订单编号高字节       
            
            // 发送响应（使用响应ID：0x1000 + 从机ID）
            status = bsp_can_Transmit(MAKE_RESPONSE_ID(slave_id), 7, resp_data);
            
            if (status == CAN_STATUS_OK) {
                DBG_INFO("CAN_SLAVE[0x%03X]: Communication response sent successfully\r\n", slave_id);
            } else {
                DBG_WARN("CAN_SLAVE[0x%03X]: Communication response send failed: %d\r\n", slave_id, status);
            }
            
        } else {
            // 订单已取件
            response_event.event_type = SLAVE_EVENT_ORDER_ALREADY_PICKED;
            DBG_INFO("CAN_SLAVE[0x%03X]: Order already picked\r\n", slave_id);
					
			// 重置状态
            slave_conn.state = SLAVE_STATE_CONNECTED;
        }
    } else if(response_event.channel_num == 0 && 
                response_event.quantity == 0 && response_event.pickup_status == 0){ 
        // 无效订单
        response_event.event_type = SLAVE_EVENT_ORDER_INVALID;
        DBG_INFO("CAN_SLAVE[0x%03X]: Invalid order\r\n", slave_id);
			
		// 重置状态
        slave_conn.state = SLAVE_STATE_CONNECTED;
    } else if(response_event.channel_num == CHANNEL_INVALID && 
                response_event.quantity == 0 && response_event.pickup_status == 0){
        // CHANNEL_INVALID --> 证明装置未分配商品通道
        response_event.event_type = SLAVE_EVENT_DEVICE_NO_CHANNEL;
        DBG_WARN("CAN_SLAVE[0x%03X]: Slave been not set channle!\r\n", slave_id);
			
		// 重置状态
        slave_conn.state = SLAVE_STATE_CONNECTED;

    }
		
		// 发送到从机的声显队列
    if (xSlaveVoiceQueue != NULL) {
        xQueueSend(xSlaveVoiceQueue, &response_event, 0);
    }
    if (xSlaveDisplayQueue != NULL) {
        xQueueSend(xSlaveDisplayQueue, &response_event, 0);
    }
		
		 return status;
}


/**************************************************************************
Function: Process command from host
Input   : slave_id - local slave ID (1-9)
          cmd      - command code
          cmd_data - command data pointer
          data_len - data length
Output  : can_status_t status
函数功能：处理主机命令
入口参数：slave_id - 本从机ID(1-9)
          cmd      - 命令码
          cmd_data - 命令数据指针
          data_len - 数据长度
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_ProcessCommand(uint16_t slave_id, uint8_t cmd, uint8_t *cmd_data, uint8_t data_len)
{
    if (!IS_VALID_SLAVE_ID(slave_id)) {
        return CAN_STATUS_INVALID_ID;
    }
    
    DBG_INFO("CAN_SLAVE[0x%03X]: Processing command 0x%02X, data_len: %d\r\n", 
           slave_id, cmd, data_len);
    
    switch (cmd) {
			
				// 处理主机回应带自身ID的命令
        case RESP_SELF_TEST:
            return app_can_Slave_ProcessPingResponse(cmd_data, data_len);
				
				// 处理主机拒绝连接命令
				case CMD_REFUSE_PING:
						//后续添加声音显示播报
						return app_can_Slave_ProcessHostRejection();
				
				// 处理主机确认收到订单命令 （从机发送订单要等待这个命令）
        case CMD_ORDER_ACK:  
            return app_can_Slave_ProcessOrderAck(cmd_data, data_len);
            
				// 处理主机通信响应命令
        case RESP_TX:
						if (slave_conn.state == SLAVE_STATE_CONNECTED) {
                slave_conn.last_confirm_time = getSysTickCnt();
            }
            return app_can_Slave_ProcessCommCommand(slave_conn.slave_id, cmd_data, data_len, can_event.qr_data);
            
        default:
            DBG_WARN("CAN_SLAVE[0x%03X]: Unknown command 0x%02X\r\n", slave_id, cmd);
            // 对于未知命令，发送错误响应（2字节）
            uint8_t error_data[2] = {0xFE, slave_id}; // 0xFE表示未知命令
            uint32_t response_id = MAKE_RESPONSE_ID(slave_id);
            return bsp_can_Transmit(response_id, 2, error_data);
    }
}


/**************************************************************************
Function: Application CAN slave receive handler
Input   : slave_id - local slave ID (1-9)
Output  : none
函数功能：应用层CAN从机接收处理函数
入口参数：slave_id - 本从机ID(1-9)
返回  值：无
**************************************************************************/
void app_can_Slave_ProcessReceive(uint16_t slave_id)
{
    if (!bsp_can_ReceiveFlag()) {
        return;
    }

    uint32_t recv_id;
    uint8_t recv_data[8];
    uint8_t recv_len;

    // 接收数据
    if (bsp_can_Receive(&recv_id, &recv_len, recv_data) != CAN_STATUS_OK) {
        return;
    }

    // 只处理发给本机的命令
    if (recv_id == slave_id && recv_len > 0) {
        DBG_INFO("CAN_SLAVE[0x%03X]: Command received from host, data_len: %d\r\n", 
               slave_id, recv_len);
        
        // 提取命令码并处理
        uint8_t cmd = recv_data[0];
        uint8_t *data = (recv_len > 1) ? &recv_data[1] : NULL;
        uint8_t data_len = (recv_len > 1) ? (recv_len - 1) : 0;
        
        app_can_Slave_ProcessCommand(slave_id, cmd, data, data_len);
    }
}

//---------------------------------------------------------------------------------------------------------------//
//--------------------------------------------从机发送二维码数据-------------------------------------------------//
//---------------------------------------------------------------------------------------------------------------//

/**************************************************************************
Function: Send QR code data to host
Input   : slave_id - local slave ID (1-9)
          order_num - order number (2 bytes)
Output  : can_status_t status
函数功能：向主机发送二维码数据
入口参数：slave_id - 本从机ID(1-9)
          order_num - 订单编号（2字节）
返回  值：can_status_t状态
**************************************************************************/
can_status_t app_can_Slave_SendQRCode(uint16_t slave_id, uint8_t *scan_data)
{
    if (!IS_VALID_SLAVE_ID(slave_id)) {
        DBG_ERROR("CAN_SLAVE: Invalid slave ID 0x%03X\r\n", slave_id);
        return CAN_STATUS_INVALID_ID;
    }
    
    // 准备发送数据（6字节：命令+ID+订单编号）
    uint8_t tx_data[7];
    tx_data[0] = CMD_TX;                 // 发送命令
    tx_data[1] = (slave_id >> 8) & 0xFF; // 从机ID高字节
    tx_data[2] = slave_id  & 0xFF; 			 // 从机ID低字节
    tx_data[3] = scan_data[0]; 					 // 订单编号0字节
    tx_data[4] = scan_data[1];					 // 订单编号1字节
    tx_data[5] = scan_data[2];					 // 订单编号2字节
    tx_data[6] = scan_data[3];					 // 订单编号3字节

    // 发送到主机（使用从机ID作为目标）
    can_status_t status = bsp_can_Transmit(MAKE_RESPONSE_ID(slave_id), 7, tx_data);
    
    if (status == CAN_STATUS_OK) {
        DBG_INFO("CAN_SLAVE[0x%03X]: QR code data sent to host, Order:%s\r\n", 
               slave_id, scan_data);
			
				// 保存待确认的订单数据和状态
        memcpy(slave_conn.pending_order_data, tx_data, 7);
        slave_conn.pending_order_length = 7;
        slave_conn.last_order_send_time = getSysTickCnt();
        slave_conn.order_retry_count = 0;
        slave_conn.state = SLAVE_STATE_WAITING_ORDER_ACK;
        app_can_UpdateSlaveLED();  // 立刻变绿色闪烁
        
        DBG_INFO("CAN_SLAVE[0x%03X]: Waiting for order ACK from host...\r\n", slave_id);
    } else {
        DBG_WARN("CAN_SLAVE[0x%03X]: QR code data send failed: %d\r\n", slave_id, status);
    }
    
    return status;
}

//---------------------------------------------------------------------------------------------------------------//
//------------------------------------------------获取状态函数---------------------------------------------------//
//---------------------------------------------------------------------------------------------------------------//

/**************************************************************************
Function: Get slave connection state
Input   : none
Output  : slave connection state
函数功能：获取从机连接状态
入口参数：无
返回  值：从机连接状态
**************************************************************************/
slave_connect_state_t app_can_Slave_GetConnectionState(void)
{
    return slave_conn.state;
}

/**************************************************************************
Function: Check if slave is connected to host
Input   : none
Output  : 1 if connected, 0 otherwise
函数功能：检查从机是否已连接到主机
入口参数：无
返回  值：1-已连接，0-未连接
**************************************************************************/
uint8_t app_can_Slave_IsConnected(void)
{
    return (slave_conn.state == SLAVE_STATE_CONNECTED);
}

/**************************************************************************
Function: Get connected host ID
Input   : none
Output  : host ID, 0 if not connected
函数功能：获取已连接的主机ID
入口参数：无
返回  值：主机ID，未连接返回0
**************************************************************************/
uint8_t app_can_Slave_GetHostID(void)
{
    return slave_conn.host_id;
}

//---------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------CAN任务------------------------------------------------------//
//---------------------------------------------------------------------------------------------------------------//

/**************************************************************************
Function: CAN slave main task
Input   : pvParameters - task parameters (should contain slave ID)
Output  : none
函数功能：CAN从机主任务
入口参数：pvParameters - 任务参数（应包含从机ID）
返回  值：无
**************************************************************************/
void can_task(void *pvParameters)
{
		// 初始化can队列
		app_can_queues_init();

		// 初始化从机连接
    app_can_Slave_Connection_Init(MY_SLAVE_ID);
    TickType_t lastWakeTime = xTaskGetTickCount();
    // 上电延迟，避免所有从机同时启动造成网络拥堵
    vTaskDelay(pdMS_TO_TICKS(MY_SLAVE_ID * 50));
                   	
    while (1) {
			
			vTaskDelayUntil(&lastWakeTime, F2T(RATE_50_HZ)); // 100ms
			
			// 管理连接状态
       app_can_Slave_ConnectionManager();
			
			// 处理CAN接收
			app_can_Slave_ProcessReceive(MY_SLAVE_ID);
					
			// 处理CAN队列中的扫码事件
			if (xCanQueue != NULL) {
					if (xQueueReceive(xCanQueue, &can_event, 0) == pdTRUE) {
							if (can_event.data_ready && can_event.is_valid_order && app_can_Slave_IsConnected()) {
									// CAN传输订单编号给主机，申请查询
									app_can_Slave_SendQRCode(MY_SLAVE_ID, can_event.qr_data);
									DBG_INFO("CAN: Data sent via CAN, Length: %d\n", can_event.data_length);
							}
					}
			}
			
    }
}

