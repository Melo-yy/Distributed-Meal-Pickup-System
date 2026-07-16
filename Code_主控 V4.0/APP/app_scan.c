#include "app_scan.h"

// 三个专用队列定义
QueueHandle_t xCanQueue = NULL;
QueueHandle_t xDisplayQueue = NULL;
QueueHandle_t xVoiceQueue = NULL;

/**************************************************************************
Function: app_scan_queues_Init
Input   : none
Output  : none
函数功能：二维码扫描相关队列初始化。创建 CAN、显示及语音三个事件队列
入口参数：无
返回  值：无
**************************************************************************/
void app_scan_queues_Init(void)
{
    // 创建三个队列，每个深度为3
    xCanQueue = xQueueCreate(3, sizeof(scan_event_t));
    xDisplayQueue = xQueueCreate(3, sizeof(scan_event_t));
    xVoiceQueue = xQueueCreate(3, sizeof(scan_event_t));
    
    if (xCanQueue == NULL || xDisplayQueue == NULL || xVoiceQueue == NULL) {
        DBG_ERROR("SCAN_QUEUES: Failed to create queues!\n");
    } else {
        DBG_INFO("SCAN_QUEUES: Three queues created successfully\n");
    }
}

 
/**************************************************************************
Function: parse_hex_string_to_binary
Input   : hex_str, binary_data, max_size
Output  : bool (success or failure)
函数功能：解析十六进制字符串（如"0x41 0x42"）为二进制字节数据
入口参数：hex_str-待解析字符串，
					binary_data-存储缓冲，
					max_size-最大容量
返回  值：true-成功解析至少一个字节
					false-格式错误或解析失败
**************************************************************************/
static bool parse_hex_string_to_binary(const char *hex_str, uint8_t *binary_data, uint8_t max_size)
{
    uint8_t byte_count = 0;
    const char *ptr = hex_str;
    
    while (*ptr && byte_count < max_size) {
        // 跳过空格和逗号
        if (*ptr == ' ' || *ptr == ',') {
            ptr++;
            continue;
        }
        //(可视字符范围：32-126，0x20-0x7E)
        // 解析 "0x41" 格式
        if (strncmp(ptr, "0x", 2) == 0 && strlen(ptr) >= 4) {
            char temp[5] = {0};
            strncpy(temp, ptr, 4);
            binary_data[byte_count++] = (uint8_t)strtol(temp, NULL, 0);
            ptr += 4;
        } else {
            break; // 格式错误
        }
    }
    
    return (byte_count > 0);
}

/**************************************************************************
Function: parse_four_hex_bytes
Input   : str, out[4]
Output  : int (status code)
函数功能：严格解析4个十六进制字节，且限制数值必须在可视字符范围内
入口参数：str-待解析字符串，
					out[4]-存储解析结果的数组
返回  值：4-成功，
					-1-格式或范围错误，-3-非可视字符，-4-字节不足或数据多余
**************************************************************************/
static int parse_four_hex_bytes(const char *str, uint8_t out[4])
{
    const char *p = str;
    uint8_t cnt = 0;

    for (int i = 0; i < 4; i++) {
        /* 跳过分隔符（除了第一个字节） */
        if (i > 0) {
            while (*p == ',' || *p == ' ' || *p == ';') {
                p++;
            }
        }

        if (*p == '\0') {
            return -4; // 字节数不足4
        }

        /* 严格检测 0x/0X 前缀 - 必须存在 */
        if (!(p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))) {
            return -1; // 缺少0x前缀
        }
        p += 2; // 跳过 "0x"

        /* 检查是否有两个十六进制数字 */
        if (!isxdigit((unsigned char)p[0]) || !isxdigit((unsigned char)p[1])) {
            return -1;
        }

        /* 转换 */
        char buf[3] = {p[0], p[1], '\0'};
        char *endptr;
        unsigned long val = strtoul(buf, &endptr, 16);
        
        if (endptr == buf || *endptr != '\0' || val > 0xFF) {
            return -1;
        }

        /* 检查是否在可视字符范围内 (0x20-0x7E) */
        if (val < 0x20 || val > 0x7E) {
            return -3;
        }

        out[cnt++] = (uint8_t)val;
        p += 2;
    }

    /* 检查是否还有多余的数据 */
    if (*p != '\0' && !isspace((unsigned char)*p) && *p != ',' && *p != ';') {
        return -4; // 有多余的数据
    }

    return 4; /* 成功：返回4 */
}

/**************************************************************************
Function: binary_to_readable_string
Input   : binary_data, length, output, output_size
Output  : none
函数功能：将二进制字节数组转换为 ASCII 可读字符串（调试用）
入口参数：binary_data-源数据，
					length-长度，
					output-输出缓冲，
					output_size-缓冲大小
返回  值：无
**************************************************************************/
static void binary_to_readable_string(const uint8_t *binary_data, uint8_t length, char *output, size_t output_size)
{
    if (output_size < length + 1) {
        return; // 缓冲区太小
    }
    
    for (int i = 0; i < length; i++) {
        output[i] = (char)binary_data[i];
    }
    output[length] = '\0';
}


/**************************************************************************
Function: FreerTOS task
Input   : none
Output  : none
函数功能：FreeRTOS二维码扫描任务
入口参数：无
返回  值：无
**************************************************************************/
void scan_task(void *pvParameters)
{ 
		DBG_INFO("扫码任务启动，等待FlashDB就绪...\n");
		/* 等待 FlashDB 初始化完成（扫码解析需要查询数据库验证订单） */
		xEventGroupWaitBits(xStartupEventGroup, BIT_FLASHDB_READY,
		                    pdFALSE, pdTRUE, portMAX_DELAY);
		DBG_INFO("扫码任务启动，等待4G业务就绪...\n");
		/* 等待 4G 业务就绪（配货单/订单下载完成后再开始扫码） */
		xEventGroupWaitBits(xStartupEventGroup, BIT_4G_BUSINESS_READY,
		                    pdFALSE, pdTRUE, portMAX_DELAY);
		DBG_INFO("扫码模块就绪，等待扫码\n");
		
		// 扫描设备对外部提供的专用队列初始化
		app_scan_queues_Init();
	
		while(1)
		{	
			/* 阻塞等待扫码帧到达信号量（ISR 释放），替代原先 10ms 轮询 recv_ok */
			if (xSemaphoreTake(xSemaphoreScan, portMAX_DELAY) == pdTRUE)
			{
				scan_event_t event = {0};  // 这里已经清零了，但后面会覆盖
				
				// 获取原始扫码数据
				char raw_scan_data[64] = {0};
				uint8_t raw_length = 0;
				event.data_ready = bsp_scan_GetData((uint8_t *)raw_scan_data, &raw_length);
				
				if (event.data_ready) {
					// 确保字符串终止
					if (raw_length < sizeof(raw_scan_data)) {
						raw_scan_data[raw_length] = '\0';
					}
					
					DBG_DEBUG("SCAN: 原始二维码数据: %s\n", raw_scan_data);
					
					// 解析十六进制字符串为二进制数据
					uint8_t binary_data[4] = {0};
					if (parse_four_hex_bytes(raw_scan_data, binary_data) == 4) {
						// 重要：先清空qr_data，再复制二进制数据
						memset(event.qr_data, 0, sizeof(event.qr_data));
						memcpy(event.qr_data, binary_data, 4);
						event.data_length = 4;  // 固定为4字节
										
						//  后续验证订单格式是否正确
						event.is_valid_order = true;
						
						// 生成可读字符串用于调试
						char readable_str[8] = {0};
						binary_to_readable_string(binary_data, 4, readable_str, sizeof(readable_str));
						DBG_DEBUG("SCAN: 转换后数据(可读): %s\n", readable_str);
						DBG_DEBUG("SCAN: 二进制数据: %02X %02X %02X %02X\n", 
							   binary_data[0], binary_data[1], binary_data[2], binary_data[3]);
						
					} else {
						// 解析失败，保持原始数据
						strncpy((char *)event.qr_data, (const char *)raw_scan_data, sizeof(event.qr_data)-1);
						event.qr_data[sizeof(event.qr_data)-1] = '\0';
						event.data_length = strlen((const char *)event.qr_data);
						event.is_valid_order = false;
						DBG_WARN("SCAN: 数据解析失败，保持原始格式\r\n");
					}
					
					// 向三个队列发送数据
					if (xCanQueue != NULL) {
						xQueueSend(xCanQueue, &event, 0);
					}
					
					if (xDisplayQueue != NULL) {
						xQueueSend(xDisplayQueue, &event, 0);
					}
					
					if (xVoiceQueue != NULL) {
						xQueueSend(xVoiceQueue, &event, 0);
					}	
				}
			}
		}	
}
