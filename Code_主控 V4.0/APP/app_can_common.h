#ifndef __CAN_APP_COMMON_H
#define __CAN_APP_COMMON_H

// ==================== ID定义区 ====================
#define HOST_ID                   0x000    // 主机ID
#define RESPONSE_BASE_ID          0x100   // 从机响应ID基础（必须与从机ID不同）

// 从机编号定义（命令目标ID）
#define SLAVE_1                   0x001
#define SLAVE_2                   0x002
#define SLAVE_3                   0x003
#define SLAVE_4                   0x004
#define SLAVE_5                   0x005
#define SLAVE_6                   0x006
#define SLAVE_7                   0x007
#define SLAVE_8                   0x008
#define SLAVE_9                   0x009

#define SLAVE_ID_MIN              SLAVE_1
#define SLAVE_ID_MAX              0x7FF

#define SLAVE_COUNT								2

// 通信协议命令定义

// ==================== 自检命令 ====================
#define CMD_SELF_TEST             0x55    // 自检命令
#define RESP_SELF_TEST            0xAA    // 自检响应
#define PING_SUCCESS							0x5A		// 自检成功响应
#define CMD_REFUSE_PING						0xA5		// 错误ID拒绝命令码

// ==================== 通信命令 ====================
#define CMD_TX             				0x88    // 发送命令
#define RESP_TX            				0xFF    // 响应发送
#define CMD_ORDER_ACK         		0x5B    // 主机收到从机订单 回传确认命令

// ================== 取件成功命令 ==================
#define CMD_PICK_OK             	0x11    // 完成取件


// 使用255（或其他特殊值）表示无效，因为通道编号通常是1-20
#define CHANNEL_INVALID           255


// ==================== 状态枚举 ====================
typedef enum {
    CAN_STATUS_OK = 0,           // 操作成功
    CAN_STATUS_TIMEOUT,          // 超时
		CAN_STATUS_NO_DATA,          // 接收FIFO空无数据
    CAN_STATUS_SEND_FAIL,        // 发送失败
    CAN_STATUS_RESP_ERROR,       // 响应错误
    CAN_STATUS_INVALID_ID,       // 无效ID
    CAN_STATUS_HW_ERROR          // 硬件错误
} can_status_t;


// ==================== 关键工具宏定义 ====================
// 判断是否为有效从机ID（命令目标）
#define IS_VALID_SLAVE_ID(id)     ((id) >= SLAVE_ID_MIN && (id) <= SLAVE_ID_MAX)

// 从响应ID提取从机编号（0x101 -> 0x001）
#define GET_SLAVE_ID_FROM_RESPONSE(resp_id)    ((resp_id) - RESPONSE_BASE_ID)

// 生成从机响应ID（0x001 -> 0x101）- 必须与从机ID不同！
#define MAKE_RESPONSE_ID(slave_id)             (RESPONSE_BASE_ID + (slave_id))

// 判断是否为从机响应ID（不是命令目标ID）
#define IS_SLAVE_RESPONSE_ID(id)  ((id) >= MAKE_RESPONSE_ID(SLAVE_ID_MIN) && \
                                  (id) <= MAKE_RESPONSE_ID(SLAVE_ID_MAX))

// 判断是否为命令目标ID（不是响应ID）
#define IS_COMMAND_TARGET_ID(id)  ((id) >= SLAVE_ID_MIN && (id) <= SLAVE_ID_MAX)

// 通用系统状态枚举 (可用于显示、声音、LED等多个模块)
typedef enum {
    // 公共状态
    STATE_NORMAL = 0,                      // 正常工作状态
    
    // 4G/系统状态
    STATE_CLOUD_ERROR = 1,                 // 云端通信异常
    STATE_LOCAL_UPDATE_FAIL = 2,           // 本地更新失败
    STATE_LOCAL_UPDATE_DONE = 3,           // 本地更新完成
    STATE_DEVICE_NO_DISPATCH_POINT = 4,    // 装置未设派送点
    STATE_NO_WAITING_DISPATCH_DATA = 5,    // 无待派送数据
    STATE_CHANNEL_NOT_SET = 6,             // 派送点未设通道
    STATE_DOWNLOAD_FAIL = 7,               // 派送下载失败
    STATE_DISPATCH_UPDATE_DONE = 8,        // 派送更新完成
    STATE_DEVICE_NO_CHANNEL = 9,           // 装置未设通道
    
    // 取餐业务状态
    STATE_RESCAN = 10,                     // 通信异常再扫码
    STATE_PROCESSING = 11,                 // 处理中等待
    STATE_PICKUP_SUCCESS = 12,             // 取件成功
    STATE_INVALID_QR = 13,                 // 无效二维码
    STATE_ALREADY_PICKED = 14,             // 已被取件
    STATE_INVALID_ORDER = 15               // 订单无效
} system_state_t;



#endif
