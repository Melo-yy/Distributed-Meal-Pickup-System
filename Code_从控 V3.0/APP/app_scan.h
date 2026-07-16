#ifndef __APP_SCAN_H
#define __APP_SCAN_H

#include "bsp_sys.h"
#include "stdlib.h"
#include "stdbool.h"
#include "ctype.h"  // 用于 isxdigit, isspace 等字符分类函数
#include "main.h"

#define SCAN_TASK_PRIO		1
#define SCAN_STK_SIZE 		256


// 扫码事件结构
typedef struct {
    bool data_ready;           // 数据就绪标志
		bool is_valid_order;       // 是否为有效订单
    uint8_t qr_data[32];       // 原始扫码数据
    uint8_t data_length;       // 数据长度
    
} scan_event_t;

// 三个专用队列
extern volatile uint8_t recv_ok;

/*-----------------------------  函数声明  -------------------------------*/
/* 队列初始化 */
void app_scan_queues_Init(void);

/* 扫描任务入口 */
void scan_task(void *pvParameters);

/* 内部工具函数（虽为 static，但声明供单文件内使用，外部无需调用） */
/* static bool parse_hex_string_to_binary(const char *hex_str, uint8_t *binary_data, uint8_t max_size); */
/* static void binary_to_readable_string(const uint8_t *binary_data, uint8_t length, char *output, size_t output_size); */

#endif
