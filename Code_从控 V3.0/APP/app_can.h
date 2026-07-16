#ifndef __APP_CAN_H
#define __APP_CAN_H

#include "bsp_can.h"
#include "app_can_common.h"
#include "main.h"


#define CAN_TASK_PRIO		5
#define CAN_STK_SIZE 		512  

// 超时时间定义
#define ORDER_RESPONSE_TIMEOUT 10000  // 10秒超时


// 订单响应事件类型
typedef enum {
    SLAVE_EVENT_ORDER_VALID = 0,      // 有效订单，等待取件
    SLAVE_EVENT_ORDER_ALREADY_PICKED, // 订单已取件
    SLAVE_EVENT_ORDER_INVALID,        // 无效订单
	  SLAVE_EVENT_ORDER_TIMEOUT,				// 超时或通信异常
		SLAVE_EVENT_DEVICE_NO_CHANNEL,    // 装置未设通道
		SLAVE_EVENT_COUNT
} order_event_type_t;



// 从机响应订单事件结构体（主机和从机通用）
typedef struct {
    order_event_type_t event_type;    // 事件类型（处理后设置）
    uint16_t slave_id;                // 从机ID
    uint8_t order_exists;             // 订单是否存在（从CAN数据解析）
    uint8_t channel_num;              // 通道编号（从CAN数据解析）
    uint8_t pickup_status;            // 取件状态（从CAN数据解析）
    uint8_t quantity;                 // 数量（从CAN数据解析）
    uint8_t order_num[4];             // 订单编号
} slave_response_event_t;


// 从机连接状态
typedef enum {
    SLAVE_STATE_DISCONNECTED = 0,  // 未连接
    SLAVE_STATE_CONNECTING,        // 连接中（已发送ping）
    SLAVE_STATE_WAITING_CONFIRM,   // 等待主机确认连接
    SLAVE_STATE_CONNECTED,         // 已连接
		SLAVE_STATE_REJECTED,          // 主机拒绝访问、
		SLAVE_STATE_WAITING_ORDER_ACK  // 等待主机确认接收订单
} slave_connect_state_t;

typedef enum {
    SLAVE_LED_EVENT_COMM_NORMAL,       // 通信正常        → 绿色常亮
    SLAVE_LED_EVENT_DATA_TRANSFER,     // 数据传输中      → 绿色闪烁
    SLAVE_LED_EVENT_NO_COMM,           // 无法通信        → 蓝色常亮
    SLAVE_LED_EVENT_COMM_ATTEMPT,      // 通信连接尝试    → 蓝色闪烁
		SLAVE_LED_EVENT_COUNT
} slave_led_event_type_t;

// Led队列专用结构体
typedef struct {
    slave_led_event_type_t event_type;
    uint16_t slave_id;                 // 可选：用于调试打印
} slave_led_event_t;

// 从机连接管理结构
typedef struct {
    slave_connect_state_t state;
    uint16_t slave_id;              // 本机ID
    uint16_t host_id;               // 存储的主机ID
    uint8_t retry_count;           // 重试计数
    uint32_t last_ping_time;       // 上次ping时间
    uint32_t last_confirm_time;    // 上次确认时间
    uint32_t last_reject_time;     // 上次被拒绝时间
    uint8_t max_retries;           // 最大重试次数
    uint16_t ping_interval;        // ping间隔(ms)
    uint16_t confirm_timeout;      // 确认超时(ms)
    uint16_t reject_wait_time;     // 拒绝后等待时间(ms)
	
		uint32_t last_order_send_time; // 上次发送订单时间
		uint8_t order_retry_count;     // 订单重试计数
		uint8_t max_order_retries;     // 最大订单重试次数
		uint16_t order_ack_timeout;    // 订单确认超时(ms)
    uint8_t pending_order_data[6]; // 待确认的订单数据
    uint8_t pending_order_length;  // 待确认订单数据长度
} slave_connection_t;


extern QueueHandle_t xCanQueue;      // CAN任务队列
extern volatile uint8_t recv_ok;


/********************************************函数声明********************************************/

/* 初始化 */
void app_can_Slave_Init(void);                      /* BSP + 过滤器初始化 */
void app_can_queues_init(void);                     /* 创建 FreeRTOS 队列 */
void app_can_Slave_Connection_Init(uint16_t slave_id);

/* 连接管理 */
void app_can_Slave_ConnectionManager(void);         /* 周期调用，含重试/超时 */

/* 主机命令处理 */
can_status_t app_can_Slave_ProcessCommand(uint16_t slave_id,
                                          uint8_t cmd,
                                          uint8_t *cmd_data,
                                          uint8_t data_len);

can_status_t app_can_Slave_ProcessPingResponse(uint8_t *cmd_data,
                                               uint8_t data_len);

can_status_t app_can_Slave_ProcessHostRejection(void);

can_status_t app_can_Slave_ProcessCommCommand(uint16_t slave_id,
                                              uint8_t *cmd_data,
                                              uint8_t data_len,
                                              uint8_t *order_num);

/* 数据发送 */
can_status_t app_can_Slave_SendPingRequest(void);
can_status_t app_can_Slave_SendPingSuccess(void);
can_status_t app_can_Slave_SendQRCode(uint16_t slave_id, uint8_t *scan_data);
can_status_t app_can_Slave_RetryOrderData(void);

/* 接收驱动 */
void app_can_Slave_ProcessReceive(uint16_t slave_id);

/* 超时管理 */
void app_can_Slave_CheckOrderTimeout(void);

/* 状态查询 */
slave_connect_state_t app_can_Slave_GetConnectionState(void);
uint8_t app_can_Slave_IsConnected(void);
uint8_t app_can_Slave_GetHostID(void);

/* 任务发送 */
static void app_can_UpdateSlaveLED(void);

/* FreeRTOS 任务入口 */
void can_task(void *pvParameters);



#endif
