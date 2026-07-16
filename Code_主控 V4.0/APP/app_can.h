#ifndef __APP_CAN_H
#define __APP_CAN_H

#include "main.h"
#include "bsp_delay.h"
#include "bsp_can.h"
#include "app_can_common.h"
#include "bsp_scan.h"
#include "bsp_voice.h"
#include "app_scan.h"
#include "app_4g.h"
#include "debug_config.h"

#define CAN_TASK_PRIO		5
#define CAN_STK_SIZE 		768 

#define ORDER_QUEUE_LENGTH 100  // 队列长度，可容纳100个请求
#define ORDER_QUEUE_ITEM_SIZE sizeof(order_request_t)

#define MAX_SLAVE_COUNT 	20  // 最大从机数量

// 超时时间定义
#define HOST_ORDER_PROCESS_TIMEOUT 10000  // 10秒超时

// 从机状态结构
typedef struct {
    uint8_t slave_id;           // 从机ID 
    uint8_t is_online;          // 在线状态
    uint32_t last_comm_time;    // 最后通信时间
} Slave_Status_t;


// 主机订单处理状态
typedef struct {
    uint8_t is_processing;           // 是否正在处理订单
    uint8_t order_id[4];             // 当前处理的订单ID
    uint32_t start_time;             // 开始处理时间
    uint8_t timeout_checked;         // 超时检查标志
} host_order_processing_t;

// 订单响应事件类型
typedef enum {
    ORDER_EVENT_VALID = 0,           // 有效订单，等待取件
    ORDER_EVENT_ALREADY_PICKED,      // 订单已取件  
    ORDER_EVENT_INVALID,             // 无效订单
	ORDER_EVENT_TIMEOUT,               // 处理超时 
    ORDER_EVENT_DEVICE_NO_CHANNEL    // 装置未设通道
} order_event_type_t;


// 响应订单事件结构体 （主机和从机通用）
typedef struct {
    order_event_type_t event_type;    // 事件类型（处理后设置）
	uint8_t Host_id;                  // 主机ID（默认0即可）
    uint8_t channel_num;              // 通道编号（从CAN数据解析）
    uint8_t quantity;                 // 数量（从CAN数据解析）
    uint8_t order_id[4];              // 订单编号
} Host_response_event_t;


// 主机从机连接状态管理
typedef enum {
    HOST_SLAVE_STATE_WAITING_CONFIRM = 0,  // 等待从机确认
    HOST_SLAVE_STATE_CONNECTED,            // 已连接
    HOST_SLAVE_STATE_DISCONNECTED          // 断开连接
} host_slave_state_t;

// 主从连接
typedef struct {
    uint8_t 						slave_id;
    host_slave_state_t 	state;
    uint8_t						  retry_count;
    uint32_t 						last_send_time;
    uint32_t 						last_confirm_time;
    uint8_t 						max_retries;
    uint16_t 						confirm_timeout;      // 确认超时时间(ms)
    uint16_t 						retry_interval;       // 重试间隔(ms)
} host_slave_connection_t;


// 订单请求结构体  （取件请求队列所用结构体）
typedef struct {
    uint16_t device_id;        // 装置ID（主机0x000或从机0x001-0x009）
    uint8_t order_id[4];       	 // 订单ID（4个可视字符）
    uint32_t timestamp;        // 请求时间戳
    uint8_t request_type;      // 请求类型：0-从机请求，1-主机扫码
} order_request_t;


extern QueueHandle_t xCanQueue;      // CAN任务队列

/********************************************函数声明********************************************/
/* 初始化 */
void app_can_Host_Init(void);
void app_can_Host_InitSlaveConnections(void);
void app_can_Host_InitSlaveIDList(uint16_t *id_list, uint8_t count);

/* 主机命令发送 */
can_status_t app_can_Host_SendCmd(uint16_t slave_id, uint8_t cmd,
                                  uint8_t *data, uint8_t data_len);

/* 从机连接管理 */
host_slave_connection_t* 	app_can_Host_FindSlaveConnection(uint16_t slave_id);
uint8_t 									app_can_Host_IsSlaveAllowed(uint16_t slave_id);
host_slave_connection_t* 	app_can_Host_AddSlaveConnection(uint16_t slave_id);
void 											app_can_Host_RemoveSlaveConnection(uint16_t slave_id);
can_status_t 							app_can_Host_SendPingResponse(uint16_t slave_id);
can_status_t 							app_can_Host_ProcessSlavePing(uint32_t recv_id,
																													uint8_t *cmd_data, uint8_t data_len);
can_status_t 							app_can_Host_ProcessPingSuccess(uint16_t slave_id,
																														uint8_t *cmd_data, uint8_t data_len);
void 											app_can_Host_ConnectionTimeoutManager(void);
uint8_t 									app_can_Host_IsSlaveOnline(uint16_t slave_id);

/* 订单/扫码相关 */
BaseType_t 		app_can_order_AddRequest(uint16_t device_id,
																				uint8_t *order_id, uint8_t request_type);
void 			 		app_can_process_host_order_request(uint8_t host_id, uint8_t *order_id);
void       		app_can_process_slave_order_request(uint16_t slave_id, uint8_t *order_id);
void       		app_can_process_host_scan_order(uint8_t *order_id);
can_status_t 	app_can_Host_SendOrderAck(uint16_t slave_id, uint8_t *order_id);
/* 超时管理 */
void app_can_Host_EndOrderProcessing(void);
void app_can_Host_StartOrderProcessing(uint8_t *order_id);
void app_can_Host_CheckOrderTimeout(void);

/* 接收主入口 */
void app_can_Host_ProcessReceive(void);

/* FreeRTOS 任务 */
void can_task(void *pvParameters);


#endif
