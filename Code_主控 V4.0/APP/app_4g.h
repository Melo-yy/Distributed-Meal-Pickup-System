#ifndef __APP_4G_H
#define __APP_4G_H 

#include "stdio.h"
#include "bsp_sys.h"
#include "app_system.h"
#include "semphr.h"      /* SemaphoreHandle_t */

//#define GPIO_PORT           GPIOC
//#define GPIO_PINS           (GPIO_Pin_6 | GPIO_Pin_7)
#define MAX_PACKET_SIZE           1024       

#define CONFIG_HTTP_GET_CMD   "config,set,http,1,uart,0,http://zd9422f6.natappfree.cc,80,,30,0,1,1,0,0\r\n"
#define CONFIG_HTTP_POST_CMD  "config,set,http,1,uart,1,http://zd9422f6.natappfree.cc,/post/order_sync,30,0,1,1,0,0\r\n"
#define HTTP_CONFIG_ACK 			"config,http,ok"
#define CONFIG_SAVE_CMD 			"config,set,save\r\n"
#define CONFIG_HTTP_INFO		  "config,get,netchaninfo,1\r\n"
#define HTTP_TASK_PRIO		5
#define HTTP_STK_SIZE 		768  
#define MAX_DEVICE_NUM 		24

/*
 * 4G 模块接收信号量：USART6 中断收到数据时释放，
 * OTA 任务等待此信号量 → 替代轮询 vTaskDelay，实现中断驱动
 */
extern SemaphoreHandle_t xSemaphore4G_RX;

typedef enum {
    HTTP_STATE_GET = 0,    // 获取订单
    HTTP_STATE_POST        // 上传/查询状态
} http_state_t;

typedef struct {
    int current_page;     // 当前页码
    int total_pages;      // 总页数
    int total_orders;     // 总订单数
    int saved_count;      // 已保存数量
    uint8_t is_completed; // 是否完成
} PaginationState;

typedef enum {
    ORDER_PROC_OK = 0,                      // 0 一切正常，可开始派送

    ORDER_PROC_PLACE_NULL = 1,              // 1 派送地点为空
    ORDER_PROC_TIME_INVALID = 2,            // 2 不在派送时间范围 或 时间格式错误
    ORDER_PROC_ALL_PICKED = 3,              // 3 订单已全部取完
    ORDER_PROC_CHANNEL_INVALID = 4,         // 4 通道状态不是1
    ORDER_PROC_FATAL_ERROR = 5,             // 5 NULL指针 or 严重异常
    ORDER_PROC_LOCAL_UPDATE_SUCCESS = 6,    // 6 本地更新完成 ✓ 新增
    ORDER_PROC_LOCAL_UPDATE_FAIL = 7,       // 7 本地更新失败 ✓ 新增
    ORDER_PROC_DATA_TRANSFER = 9,           // 8 数据传输中

    ORDER_PROC_CLOUD_TIME_FAIL = 10,        // 10 云端时间请求失败
    ORDER_PROC_ORDER_DOWNLOAD_SUCCESS = 11, // 11 派送数据订单下载成功
    ORDER_PROC_ORDER_DOWNLOAD_FAIL = 12,    // 12 派送数据订单下载失败
    ORDER_PROC_DELIVERY_ID_MISMATCH = 13,   // 13 派送地点不匹配

    ORDER_PROC_LOCAL_INFO_NONE = 20,        // 20 本地没有派送信息（首次启动）
    ORDER_PROC_UNKNOWN_STATE = 99,          // 99 其他未知错误
		ORDER_PROC_FINISH = 100
} order_proc_state_t;

typedef enum{
    STATUS_4G_DISCONNECTED = 0,     // 无法通信
    STATUS_4G_CONNECTING,           // 尝试连接
    STATUS_4G_NORMAL,               // 通信正常
    STATUS_4G_TRANSFER              // 数据传输中
} order_proc_4G_status_t;

typedef struct {
    order_proc_state_t state;           // 订单处理状态枚举
    order_proc_4G_status_t state_4G;
    //char order_id[16];                // 订单号（可选）
} order_proc_event_t;


typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} SystemTime_t;



typedef struct {
    uint16_t list[MAX_DEVICE_NUM];
    uint8_t  count;
} device_id_mgr_t;

//device存储管理
/* 全局设备管理器 */
extern device_id_mgr_t g_device_mgr;

/* 接口函数 */
void DeviceMgr_Init(void);
void DeviceMgr_Clear(void);
int  DeviceMgr_Add(uint16_t device_id);
int DeviceMgr_Export(uint16_t *out_array, uint8_t max_len);


void HTTP_task(void *pvParameters);

int RequestAndSaveDeviceProducts(void);
int GetAllOrdersWithPagination(void);//更新订单信息

#endif

