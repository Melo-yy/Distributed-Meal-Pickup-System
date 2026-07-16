#include "stm32f4xx.h"
#include "app_4g.h"
#include "debug_config.h"
#include "event_groups.h"
#include "task.h"

QueueHandle_t xOrderProc4G_Show_Queue = NULL;
QueueHandle_t xOrderProc4G_LED_Queue = NULL;
QueueHandle_t xOrderProc4G_Voice_Queue = NULL;
SystemTime_t g_SystemTime;   // 全局时间存储变量

uint8_t http_rx_buffer[MAX_PACKET_SIZE];
uint8_t http_info_buf[MAX_PACKET_SIZE];
uint8_t packet_ready = 0;
uint16_t packet_index = 0;
uint8_t receiving = 0;

uint8_t OK_4G_flag = 0;

device_id_mgr_t g_device_mgr;
PaginationState page_order_state = {0};
/**************************************************************************
Function: DeviceMgr_Init
Input   : none
Output  : none
函数功能：设备管理器初始化函数。系统启动时调用一次，清空设备管理器结构体
入口参数：无
返回  值：无
**************************************************************************/
void DeviceMgr_Init(void)
{
    // 将整个设备管理器结构体清雀
    memset(&g_device_mgr, 0, sizeof(g_device_mgr));
}

/**************************************************************************
Function: DeviceMgr_Clear
Input   : none
Output  : none
函数功能：清空设备列表。将设备计数器设置为0，表示当前没有管理的设备
入口参数：无
返回  值：无
**************************************************************************/
void DeviceMgr_Clear(void)
{
    g_device_mgr.count = 0;  // 设备数量归零
}

/**************************************************************************
Function: DeviceMgr_Add
Input   : device_id - 要添加的设备ID，6位无符号整数，
Output  : 返回添加结果（表示成功（1表示失败（列表已满）
函数功能：向设备管理器中添加一个新的设备ID。用于记录当前管理的所有设备
入口参数：device_id - 设备ID
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int DeviceMgr_Add(uint16_t device_id)
{
    // 检查设备数量是否已达到上限
    if (g_device_mgr.count >= MAX_DEVICE_NUM)
        return -1;  // 列表已满，返回失败

    // 将设备ID添加到列表中，并增加计数器
    g_device_mgr.list[g_device_mgr.count++] = device_id;
    return 0;  // 返回成功
}

/**************************************************************************
Function: DeviceMgr_Export
Input   : out_array - 输出缓冲区指针，用于接收设备ID列表
          max_len - 输出缓冲区的最大长庀
Output  : 返回实际复制的设备数量，-1表示参数错误
函数功能：导出设备管理器中的所有设备ID到指定的输出缓冲匀
入口参数：out_array - 输出缓冲区指针
          max_len - 缓冲区最大可容纳元素个数
返回  值：int - 实际复制的设备数量，-1:参数错误
**************************************************************************/
int DeviceMgr_Export(uint16_t *out_array, uint8_t max_len)
{
    // 检查输出缓冲区是否有效
    if (out_array == NULL)
        return -1;  // 无效指针返回错误

    // 确定要复制的数量（不超过缓冲区大小和设备总数，
    uint8_t copy_cnt = g_device_mgr.count;
    if (copy_cnt > max_len)
        copy_cnt = max_len;  // 截断到缓冲区大小

    // 循环复制设备ID到输出缓冲区
    for (uint8_t i = 0; i < copy_cnt; i++) {
        out_array[i] = g_device_mgr.list[i];
    }

    return copy_cnt;  // 返回实际复制的数釀
}

/**************************************************************************
Function: USART6_IRQHandler
Input   : none
Output  : none
函数功能：串发中断处理函数。接攀G模块返回的HTTP响应数据，
          将数据存入环形缓冲区，并检测是否接收完成
入口参数：无
返回  值：无
**************************************************************************/
void USART6_IRQHandler(void)
{
    uint8_t Usart6_Receive;  // 定义接收字节变量

    // 判断是否接收到数据（RXNE中断标志（
    if (USART_GetITStatus(USART6, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART6, USART_IT_RXNE); // 清除中断标志佀
        Usart6_Receive = USART_ReceiveData(USART6);     // 读取接收到的数据

        // 将接收到的字节存入缓冲区
        http_rx_buffer[packet_index++] = Usart6_Receive;

        // 释放信号量通知 OTA 任务：有数据到达，不用轮询等亀
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xSemaphore4G_RX, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        // 防止缓冲区溢出，如果索引超过最大值则回绕分
        if (packet_index >= MAX_PACKET_SIZE)
            packet_index = 0; // 防止溢出

        // 可选：检测是否接收完成（可以根据结尾判断，比如\r\n（
//        if (Usart6_Receive == '\n') 
//        {
//            http_rx_buffer[packet_index] = '\0'; // 字符串结射
//            packet_ready = 1;
//        }
    }
}

/**************************************************************************
Function: uart6_receiver_clear
Input   : len - 要清除的缓冲区长庀
Output  : none
函数功能：清空串发接收缓冲区。将指定长度的缓冲区内容清零，并重置接收索引
入口参数：len - 要清除的缓冲区长庀
返回  值：无
**************************************************************************/
void uart6_receiver_clear(uint16_t len)
{
    // 将接收缓冲区清零
    memset(http_rx_buffer, 0x00, len);
    packet_index = 0;  // 重置接收索引
}

/**************************************************************************
Function: uart6_strstr
Input   : ack - 要匹配的应答字符串指针
Output  : 返回匹配结果，表示匹配成功：表示匹配失败
函数功能：在串口接收缓冲区中搜索指定的应答字符串，匹配成功后清空缓冲匀
入口参数：ack - 要搜索的字符一
返回  值：int - 1:匹配成功 0:匹配失败
**************************************************************************/
int uart6_strstr(const char *ack)
{
    // 检查输入参数有效怀
    if (ack == NULL)
        return 0;

    // 在接收缓冲区中搜索匹配字符串
    if (strstr((const char *)http_rx_buffer, ack))
    {
        uart6_receiver_clear(MAX_PACKET_SIZE);  // 匹配成功，清空缓冲区
        return 1; // 匹配成功
    }

    uart6_receiver_clear(MAX_PACKET_SIZE);  // 匹配失败，清空缓冲区
    return 0;  // 匹配失败
}

/**
 * @brief  从串口接收到的原始数据中提取并存储所有订區
 * @param  recv_str: 串口缓冲区内容，例如:
 *         "{2,ORD002,P200,5,0,1762572000}\r\n{3,ORD003,P300,1,1,1762580700}\r\n"
 * @retval 成功存入的订单数釀
 */
int FlashDB_SaveParsedOrders(const char *input) 
{
    // 检查输入参数有效怀
    if (input == NULL || strlen(input) == 0) return 0;

    const char *start = input;  // 指向当前解析位置
    const char *end = NULL;      // 指向结束花括发
    char temp[128];              // 临时缓冲区，存储单条记录
    int saved_count = 0;          // 成功保存计数器

    // 循环查找所有的起始花括友
    while ((start = strchr(start, '{')) != NULL) {
        end = strchr(start, '}');  // 查找对应的结束花括号
        if (!end) break; // 没有完整结束符，等下次数据补全

        // 计算单条记录的长度（包含花括号）
        size_t len = end - start + 1;
        if (len >= sizeof(temp)) len = sizeof(temp) - 1;  // 防止缓冲区溢净

        // 复制单条完整记录到临时缓冲区
        strncpy(temp, start, len);
        temp[len] = '\0';  // 添加字符串结束符

        order_t order;  // 定义订单结构佀
        memset(&order, 0, sizeof(order));  // 清空结构佀

        // 去掉花括号解析数据
        char *p = temp + 1;  // 跳过起始花括友
        char *token = strtok(p, ",");  // 以逗号分割
        int field = 0;  // 字段计数器

        // 循环解析各个字段
        while (token != NULL) {
            switch (field) {
                case 0:  // 第一个字段：PK_ID
                    strncpy(order.PK_ID, token, sizeof(order.PK_ID) - 1);
                    order.PK_ID[sizeof(order.PK_ID) - 1] = '\0'; 
                    break;
                case 1:  // 第二个字段：order_id
                    strncpy(order.order_id, token, sizeof(order.order_id) - 1);
                    order.order_id[sizeof(order.order_id) - 1] = '\0'; 
                    break;
                case 2:  // 第三个字段：product_id
                    order.product_id = (uint8_t)atoi(token); 
                    break;
                case 3:  // 第四个字段：quantity
                    order.quantity = (uint8_t)atoi(token); 
                    break;
                case 4:  // 第五个字段：status
                    order.status = (uint8_t)atoi(token); 
                    break;
                case 5:  // 第六个字段：time_point
                    order.time_point = (uint32_t)strtoul(token, NULL, 10); 
                    break;
                default: 
                    break;
            }
            token = strtok(NULL, ",");  // 继续解析下一个字段
            field++;
        }

        // 如果解析到至射个字段，说明数据完整
        if (field >= 6) {
            // 保存订单到FlashDB
            if (FlashDB_SaveOrder(&order) == 0) {
                saved_count++;  // 成功计数器加1
                // 打印调试信息
                DBG_DEBUG("save: PK_ID=%s, Order=%s, Product=%u, Qty=%u, Status=%u, Time=%u\r\n",
                       order.PK_ID, order.order_id, order.product_id, order.quantity,
                       order.status, order.time_point);
            }
        } else {
            DBG_WARN("saved fail: %s\r\n", temp);  // 数据不完整，保存失败
        }

        start = end + 1; // 继续查找下一条记当
    }

    return saved_count; // 返回保存的订单数釀
}

// ============================
// 1. 分页初始化函数
// ============================
void Pagination_Init(void) 
{
    // 清空分页状态结构体
    memset(&page_order_state, 0, sizeof(page_order_state));
    page_order_state.current_page = 1;  // 设置当前页为笀页
    DBG_INFO("[Pagination] Initialized\r\n");  // 打印初始化信息
}

// ============================
// 2. 解析分页信息备
// ============================
int ParsePageHeader(const char *response) 
{
    // 查找分页信息备 {PAGE:1/5,TOTAL:37}
    const char *header_start = strstr(response, "{PAGE:");
    if (header_start == NULL) {
        DBG_WARN("[Pagination] No page header found\r\n");  // 未找到分页头
        return -1;
    }
    
    // 解析分页信息：当前页/总页数，总订单数
    int parsed = sscanf(header_start, "{PAGE:%d/%d,TOTAL:%d}", 
                       &page_order_state.current_page, 
                       &page_order_state.total_pages, 
                       &page_order_state.total_orders);
    
    if (parsed == 3) {  // 成功解析3个字段
        DBG_INFO("[Pagination] Page %d/%d, Total: %d orders\r\n", 
               page_order_state.current_page, 
               page_order_state.total_pages, 
               page_order_state.total_orders);
        return 0;  // 返回成功
    }
    
    return -1;  // 解析失败
}

// ============================
// 3. 支持分页的订单保存函数
// ============================
int FlashDB_SaveParsedOrders_Paginated(const char *input) 
{
    // 检查输入参数有效怀
    if (input == NULL || strlen(input) == 0) return 0;
    
    const char *start = input;  // 指向当前解析位置
    const char *end = NULL;      // 指向结束花括发
    char temp[128];              // 临时缓冲匀
    int saved_count = 0;          // 成功保存计数器
    
    // 跳过分页信息备
    // 找到第一个结束的}（分页头的结束）
    const char *page_header_end = strstr(input, "}");
    if (page_header_end != NULL) {
        start = page_header_end + 1;  // 从分页头之后开始解析订单数据
    }
    
    // 解析订单数据（与FlashDB_SaveParsedOrders类似（
    while ((start = strchr(start, '{')) != NULL) {
        end = strchr(start, '}');
        if (!end) break; // 没有完整结束笀
        
        // 计算单条记录长度
        size_t len = end - start + 1;
        if (len >= sizeof(temp)) len = sizeof(temp) - 1;
        
        // 复制单条记录
        strncpy(temp, start, len);
        temp[len] = '\0';
        
        order_t order;
        memset(&order, 0, sizeof(order));
        
        // 解析各字段（去掉花括号）
        char *p = temp + 1;  // 跳过开始的{
        char *token = strtok(p, ",");
        int field = 0;
        
        while (token != NULL) {
            switch (field) {
                case 0: 
                    strncpy(order.PK_ID, token, sizeof(order.PK_ID) - 1);
                    order.PK_ID[sizeof(order.PK_ID) - 1] = '\0'; 
                    break;
                case 1: 
                    // order_identify_id 现在映个字符的字符一
                    strncpy(order.order_id, token, sizeof(order.order_id) - 1);
                    order.order_id[sizeof(order.order_id) - 1] = '\0'; 
                    break;
                case 2: 
                    order.product_id = (uint8_t)atoi(token); 
                    break;
                case 3: 
                    order.quantity = (uint8_t)atoi(token); 
                    break;
                case 4: 
                    order.status = (uint8_t)atoi(token); 
                    break;
                case 5: 
                    order.time_point = (uint32_t)strtoul(token, NULL, 10); 
                    break;
                default: 
                    break;
            }
            token = strtok(NULL, ",");
            field++;
        }
        
        if (field >= 6) {  // 数据完整
            if (FlashDB_SaveOrder(&order) == 0) {
                saved_count++;
                DBG_DEBUG("  [Save] PK_ID=%s, OrderID=%s, Product=%u, Qty=%u\r\n",
                       order.PK_ID, order.order_id, order.product_id, order.quantity);
            }
        } else {
            DBG_WARN("  [Save Fail] Incomplete data: %s\r\n", temp);
        }
        
        start = end + 1; // 继续找下一杀
    }
    
    return saved_count;  // 返回保存数量
}

// ============================
// 4. 分页请求函数（核心）
// ============================
int RequestAndSaveOrderInfo_Paginated(void) 
{
    char url[128];  // URL缓冲匀
    int saved_in_page = 0;  // 当前页保存的订单数
    
    // 如果是第一次请求或需要重新开姀
    if (page_order_state.current_page == 0) {
        page_order_state.current_page = 1;  // 从第1页开姀
    }
    
    // 构建带分页参数的URL
    // 格式: /get/order_info?page=X&page_size=5
    sprintf(url, "/get/order_info?page=%d&page_size=5", page_order_state.current_page);
    
    DBG_INFO("[Pagination] Requesting page %d...\r\n", page_order_state.current_page);
    
    // 发送HTTP请求
    USART_4G_SendString(url);
xSemaphoreTake(xSemaphore4G_RX, pdMS_TO_TICKS(3000));
    vTaskDelay(pdMS_TO_TICKS(200));  // 等后续字节收完
    
    // 复制响应数据到缓冲区
    memcpy(http_info_buf, http_rx_buffer, MAX_PACKET_SIZE);
    uart6_receiver_clear(MAX_PACKET_SIZE);  // 清空接收缓冲匀
    
    DBG_DEBUG("[Pagination] Received %d bytes\r\n", strlen((char*)http_info_buf));
    
    // 解析分页信息备
    if (ParsePageHeader((const char *)http_info_buf) == 0) {
        // 保存订单数据（跳过已解析的分页头（
        saved_in_page = FlashDB_SaveParsedOrders_Paginated((const char *)http_info_buf);
        
        if (saved_in_page > 0) {
            page_order_state.saved_count += saved_in_page;  // 累加保存总数
            DBG_INFO("[Pagination] Page %d: Saved %d orders, Total saved: %d\r\n", 
                   page_order_state.current_page, saved_in_page, page_order_state.saved_count);
            
            // 检查是否还有下一页
            if (page_order_state.current_page < page_order_state.total_pages) {
                page_order_state.current_page++;  // 页码加
                return 1;  // 返回1表示还有更多数据
            } else {
                page_order_state.is_completed = 1;  // 所有页已获取完成
                DBG_INFO("[Pagination] All pages completed! Total: %d orders\r\n", 
                       page_order_state.saved_count);
                return 0;  // 返回0表示所有数据完成
            }
        } else {
            DBG_WARN("[Pagination] Page %d: No orders saved\r\n", page_order_state.current_page);
            return -1;  // 返回-1表示错误
        }
    } else {
        DBG_ERROR("[Pagination] Failed to parse page header\r\n");
        return -1;  // 返回-1表示错误
    }
}

// ============================
// 5. 获取所有订单（主函数）
// ============================
int GetAllOrdersWithPagination(void) 
{
    int retry_count = 0;  // 重试计数器
    const int max_retries = 3;  // 最大重试次数
    
    // 初始化分页状态
    Pagination_Init();
    
    DBG_INFO("[System] Starting to fetch all orders with pagination...\r\n");
    
    // 循环获取所有页
    while (!page_order_state.is_completed && retry_count < max_retries) {
        int result = RequestAndSaveOrderInfo_Paginated();  // 请求当前页
        
        if (result == 0) {
            // 所有数据获取完成
            DBG_INFO("[System] All orders fetched successfully!\r\n");
            DBG_INFO("[System] Total orders saved: %d\r\n", page_order_state.saved_count);
            return page_order_state.saved_count;  // 返回保存总数
            
        } else if (result == 1) {
            // 还有更多数据，重置重试计数
            retry_count = 0;
            vTaskDelay(pdMS_TO_TICKS(500));  // 等待一下再请求下一页
            
        } else {
            // 请求失败，重诀
            retry_count++;
            DBG_WARN("[System] Request failed, retry %d/%d\r\n", retry_count, max_retries);
            vTaskDelay(pdMS_TO_TICKS(1000 * retry_count));  // 指数退遀
        }
    }
    
    if (retry_count >= max_retries) {
        DBG_ERROR("[System] Failed after %d retries\r\n", max_retries);
        return -1;  // 超过最大重试次数，返回失败
    }
    
    return page_order_state.saved_count;  // 返回保存总数
}

/**************************************************************************
Function: HTTP_send_config
Input   : str - 要发送的配置命令字符一
          ack - 期望的应答字符串
          waittime - 等待应答的超时时间（单位（00ms（
Output  : 返回发送结果，0表示成功：表示失败或超无
函数功能：发送HTTP配置命令并等待应答。用于配罀G模块的HTTP参数
入口参数：str - 配置命令
          ack - 期望的应筀
          waittime - 等待超时时间
返回  值：uint8_t - 0:成功 1:失败/超时
**************************************************************************/
int HTTP_send_config(const char *str, const char *ack, uint16_t waittime)
{
    uint8_t res = 0;  // 结果标志，表示成功

    if (str == NULL)
        return 1;  // 无效参数返回失败

    USART_4G_SendString(str); // 发送命什

    // 如果需要等待应筀
    if (ack && waittime) 
    {
        while (--waittime)  // 递减等待时间
        {
            vTaskDelay(pdMS_TO_TICKS(100));  // 毀00ms检查一欀
            if (uart6_strstr(ack))  // 检查是否收到期望的应答
            {
                break; // 成功接收到匹配字符串，退出循玀
            }
        }

        if (waittime == 0)  // 超时
            res = 1; // 超时未接收到应答
    }

    return res;  // 返回结果
}

/**************************************************************************
Function: HTTP_Module_Init
Input   : str - HTTP配置命令字符一
Output  : 返回初始化结果，0表示成功：表示失败
函数功能：初始化4G模块的HTTP配置。发送配置命令并等待应答，失败时自动重试
入口参数：str - HTTP配置命令
返回  值：uint8_t - 0:成功 1:失败
**************************************************************************/
uint8_t HTTP_Module_Init(const char *str)
{
    uint8_t retry = 0;  // 重试计数器
    uint8_t result = 1;  // 结果标志，表示失败

    DBG_INFO("Init 4G HTTP Config\n");

    // 清空接收缓冲匀
    uart6_receiver_clear(MAX_PACKET_SIZE);

    // 重试最大0欀
    while (retry < 10)
    {
        DBG_DEBUG("Send HTTP Config(no.%d )\n", retry + 1);

        // 发送HTTP配置命令并等待ACK
        if (HTTP_send_config(str, HTTP_CONFIG_ACK, 5) == 0)
        {
            // 发送保存配置命什
            if (HTTP_send_config(CONFIG_SAVE_CMD, "config,save,ok", 5) == 0)
            {
                DBG_INFO("HTTP Configing %s...!\r\n", str);
                vTaskDelay(pdMS_TO_TICKS(8000));  // 等待配置生效
                DBG_INFO("HTTP Config Success!\r\n");
                result = 0;  // 配置成功
                break;
            }
        }   
        else
        {
            DBG_WARN("HTTP Config Fail,Retrying\n");
            retry++;  // 重试计数加
            vTaskDelay(pdMS_TO_TICKS(100));  // 等待后重诀
        }
    }

    if (result)
    {
        DBG_ERROR("HTTP Config Fail\n");  // 最终配置失败
    }

    return result;  // 返回结果
}

/**************************************************************************
Function: ParseAndSaveDeliveryInfo
Input   : rx - 接收到的原始数据字符一
Output  : 返回保存结果，表示成功：1表示失败
函数功能：解析并保存派送信息。从接收到的数据中提发2个字段并存入结构体，
          最后保存到FlashDB一
入口参数：rx - 原始数据字符一
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int ParseAndSaveDeliveryInfo(const char *rx)
{
    DeliveryInfo info;  // 定义派送信息结构体
    memset(&info, 0, sizeof(info));  // 清空结构佀

    // 临时变量，用于存储解析结析
    char PK_ID[10];
    char delivery_id[10];
    char delivery_date[12];
    char delivery_time[4];
    char start_time[6];
    char end_time[6];
    int order_count;
    int picked_count;
    int order_status;
    char status_sync_time[10];
    int channel_status;
    char device_time[10];

    /* 使用 sscanf 按顺序解析12 个字段*/
    int ret = sscanf(rx,
        "{%9[^,],%9[^,],%11[^,],%3[^,],%5[^,],%5[^,],%d,%d,%d,%9[^,],%d,%9[^}]}",
        PK_ID,
        delivery_id,
        delivery_date,
        delivery_time,
        start_time,
        end_time,
        &order_count,
        &picked_count,
        &order_status,
        status_sync_time,
        &channel_status,
        device_time
    );

    // 检查是否成功解析2个字段
    if (ret != 12) {
        DBG_ERROR("解析 DeliveryInfo 失败！字段数量不正确，ret=%d\r\n", ret);
        return -1;  // 解析失败
    }

    /* 复制解析结果到结构体 */
    strcpy(info.PK_ID, PK_ID);
    strcpy(info.delivery_id, delivery_id);
    strcpy(info.delivery_date, delivery_date);
    strcpy(info.delivery_time, delivery_time);
    strcpy(info.start_time, start_time);
    strcpy(info.end_time, end_time);

    info.order_count  = order_count;
    info.picked_count = picked_count;
    info.order_status = order_status;

    strcpy(info.status_sync_time, status_sync_time);
    info.channel_status = channel_status;
    strcpy(info.device_time, device_time);

    /* 存入 FlashDB */
    return SaveDeliveryInfo(&info);   
}

/**************************************************************************
Function: CheckDeliveryIDEqual
Input   : rx - 接收到的原始数据字符一
          compare_id - 要比较的派送ID
Output  : 返回匹配结果，具体返回值见函数内说映
函数功能：比较接收到的派送ID与本地存储的是否一致，并判断通道状态
          返回值用于后续流程控分
入口参数：rx - 原始数据
          compare_id - 要比较的ID
返回  值：1:ID匹配且通道需要更新2:ID匹配但通道无需更新
          3:ID不匹配但通道需更新 4:ID不匹配且通道无需更新
          -1:解析失败
**************************************************************************/
int CheckDeliveryIDEqual(const char *rx, const char *compare_id)
{
    char delivery_id[16] = {0};  // 存储解析出的派送ID
    int channel_status = -1;      // 存储解析出的通道状态
    /*
     * 数据格式示例，
     * {PK_001,ZHK-HZ-02,2025/11/10,1,9:20,22:30,10,0,0,11:20,1,12:00}
     */
    /* 解析 delivery_id（第 2 个字段）咀channel_status（第 11 个字段） */
    int ret = sscanf(rx,
                     "{%*[^,],%15[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d",
                     delivery_id,
                     &channel_status);

    // 检查解析结析
    if (ret != 2) {
        DBG_ERROR("Parse delivery_id/channel_status FAIL! ret=%d\r\n", ret);
        return -1;  // 解析失败
    }

    int id_match = (strcmp(delivery_id, compare_id) == 0);  // ID是否匹配
    int ch_ok    = (channel_status == 1);  // 通道状态是否为1

    // 根据匹配结果返回不同状态码
    if (id_match && ch_ok)  // ID匹配，通道数据需要更新
        return 1;

    if (id_match && !ch_ok)  // ID匹配，通道数据不需要更新
        return 2;

    if (!id_match && ch_ok)  // ID不匹配，但通道需要更新
        return 3;

    /* !id_match && !ch_ok */// ID不匹配，通道不需要更新
    return 4;
}

/**************************************************************************
Function: RequestAndCheckDeliveryID
Input   : match_delivery_id - 要匹配的派送ID
Output  : 返回CheckDeliveryIDEqual的解析结析
函数功能：查询服务器并检查派送地点ID。发送请求后解析响应，匹配派送ID
入口参数：match_delivery_id - 要匹配的派送ID
返回  值：int - CheckDeliveryIDEqual的返回值
**************************************************************************/
int RequestAndCheckDeliveryID(const char *match_delivery_id)
{
    // 发送请汀
    USART_4G_SendString("/get/delivery_places");
    xSemaphoreTake(xSemaphore4G_RX, pdMS_TO_TICKS(5000));
    vTaskDelay(pdMS_TO_TICKS(200));

    // 复制接收缓冲匀
    memcpy(http_info_buf, http_rx_buffer, MAX_PACKET_SIZE);
    uart6_receiver_clear(MAX_PACKET_SIZE);  // 清空接收缓冲匀

    // 使用 CheckDeliveryIDEqual 解析并匹酀
    int result = CheckDeliveryIDEqual((const char *)http_info_buf, match_delivery_id);

    // 根据结果打印日志
    if (result == 1) {
        DBG_INFO("[DeliveryPlaces] delivery_id Match successful!\r\n");
    } 
    else if (result == 0) {
        DBG_WARN("[DeliveryPlaces] delivery_id Match Fail!\r\n");
    } 
    else {
        DBG_ERROR("[DeliveryPlaces] Analysls Fail!\r\n");
    }

    return result;  // 返回解析结果
}

/**************************************************************************
Function: ParseAndSave_DeviceProduct
Input   : uart_data - 串口接收到的原始数据
Output  : 返回成功保存的设备商品对应关系数量，负值表示错诀
函数功能：解析装罀商品对应关系并保存。从数据中提取多个{device,product}对，
          依次保存到FlashDB中，并添加到设备管理器
入口参数：uart_data - 原始数据字符一
返回  值：int - 成功保存数量：1:解析错误，2:保存错误
**************************************************************************/
int ParseAndSave_DeviceProduct(const char *uart_data)
{
    // 检查输入参数有效怀
    if (uart_data == NULL || uart_data[0] == '\0') {
        DBG_WARN("ParseDeviceProduct: UART data empty!\r\n");
        return 0;  // 数据为空，返回
    }

    const char *p = uart_data;  // 指向当前解析位置
    int saved_count = 0;  // 成功保存计数器

    // 循环查找所有的起始花括友
    while ((p = strchr(p, '{')) != NULL)
    {
        p++;  // 跳过 '{'

        char device_id_str[16] = {0};  // 设备ID字符一
        uint16_t device_id = 0;        // 设备ID数值
        char product_str[16] = {0};    // 产品ID字符一
        uint16_t product_id = 0;        // 产品ID数值

        /* ------- 解析 device_id ------- */
        const char *comma = strchr(p, ',');  // 查找逗号分隔笀
        if (!comma) {
            DBG_ERROR("Parse ERR: Missing comma!\r\n");
            return -1;  // 解析错误
        }

        // 提取设备ID字符一
        size_t id_len = comma - p;
        if (id_len >= sizeof(device_id_str)) id_len = sizeof(device_id_str) - 1;
        strncpy(device_id_str, p, id_len);
        device_id_str[id_len] = '\0';

        // 转换为数值
        device_id = (uint16_t)atoi(device_id_str);

        /* ------- 解析 product_id（整数） ------- */
        const char *right = strchr(comma + 1, '}');  // 查找结束花括发
        if (!right) {
            DBG_ERROR("Parse ERR: Missing '}'!\r\n");
            return -1;  // 解析错误
        }

        // 提取产品ID字符一
        size_t product_len = right - (comma + 1);
        if (product_len >= sizeof(product_str)) product_len = sizeof(product_str) - 1;
        strncpy(product_str, comma + 1, product_len);
        product_str[product_len] = '\0';

        // 转换为数值
        product_id = (uint16_t)atoi(product_str);

        /* ------- 保存分FlashDB ------- */
        DBG_DEBUG("Saving: dev=%u , product_id=%u\r\n", device_id, product_id);

        int ret = FlashDB_SaveDeviceProduct(device_id, product_id);
        if (ret != 0) {
            DBG_ERROR("Save ERR: dev=%u\r\n", device_id);
            return -2;  // 保存错误
        }
        
        /* ⭀保存 device_id 到全局管理器*/
        DeviceMgr_Add(device_id);

        saved_count++;  // 成功保存计数加
        p = right + 1; // 继续查找下一一{ ... }
    }

    return saved_count;  // 返回成功保存数量
}

/**************************************************************************
Function: RequestAndSaveDeliveryPlaces
Input   : none
Output  : 返回保存结果，表示成功：表示失败
函数功能：请求并保存派送地点信息。发送HTTP请求后，解析并保存派送信息到FlashDB
入口参数：无
返回  值：int - 0:成功 1:失败
**************************************************************************/
int RequestAndSaveDeliveryPlaces(void)
{
    // 发送请汀
    USART_4G_SendString("/get/delivery_places");
    xSemaphoreTake(xSemaphore4G_RX, pdMS_TO_TICKS(5000));
    vTaskDelay(pdMS_TO_TICKS(200));

    // 拷贝缓冲匀
    memcpy(http_info_buf, http_rx_buffer, MAX_PACKET_SIZE);
    uart6_receiver_clear(MAX_PACKET_SIZE);  // 清空接收缓冲匀

    // 解析 + 保存
    int saved = ParseAndSaveDeliveryInfo((const char *)http_info_buf);

    if (saved == 0) {
        DBG_INFO("[DeliveryPlaces] Save OK\r\n");
        return 0;  // 成功
    } else {
        DBG_ERROR("[DeliveryPlaces] GET FAIL!\r\n");
        return 1;  // 失败
    }
}

/**************************************************************************
Function: RequestAndSaveDeviceProducts
Input   : none
Output  : 返回保存结果，成功返回保存数量，负值表示错诀
函数功能：请求并保存装置-商品配置。发送HTTP请求后，解析并保存设备商品对应关系
入口参数：无
返回  值：int - 成功:保存数量 -1:接收空-2:解析错误 -3:保存错误
**************************************************************************/
int RequestAndSaveDeviceProducts(void)
{
    // 发送请汀
    USART_4G_SendString("/get/device_product");
    xSemaphoreTake(xSemaphore4G_RX, pdMS_TO_TICKS(5000));
    vTaskDelay(pdMS_TO_TICKS(200));

    // 拷贝接收缓冲匀
    memcpy(http_info_buf, http_rx_buffer, MAX_PACKET_SIZE);
    uart6_receiver_clear(MAX_PACKET_SIZE);  // 清空接收缓冲匀

    // 检查是否收到数据
    if (http_info_buf[0] == '\0') {
        DBG_ERROR("RequestDeviceProduct ERR: RX empty!\r\n");
        return -1;   // 串口无数据
    }

    DBG_DEBUG("receiver device_product: %s\r\n", http_info_buf);

    // 解析并保存
    int saved = ParseAndSave_DeviceProduct((const char *)http_info_buf);

    // 根据结果返回不同状态
    if (saved > 0) {
        DBG_INFO("DeviceProduct Saved %d items\r\n", saved);
        return saved;  // 返回保存数量
    }
    else if (saved == 0) {
        DBG_WARN("DeviceProduct: No valid data\r\n");
        return 0;  // 无有效数据
    }
    else if (saved == -1) {
        DBG_ERROR("DeviceProduct Parse ERR!\r\n");
        return -2;  // 解析错误
    }
    else {
        DBG_ERROR("DeviceProduct Save ERR!\r\n");
        return -3;  // 保存错误
    }
}

/**************************************************************************
Function: RequestAndSaveOrderInfo
Input   : none
Output  : 返回保存的订单数量，负值表示失败
函数功能：请求并保存订单信息。发送HTTP请求后，解析并保存所有订单数据
入口参数：无
返回  值：int - 成功:保存数量 -1:失败
**************************************************************************/
int RequestAndSaveOrderInfo(void)
{
    // 发送请汀
    USART_4G_SendString("/get/order_info");
    xSemaphoreTake(xSemaphore4G_RX, pdMS_TO_TICKS(5000));
    vTaskDelay(pdMS_TO_TICKS(200));

    // 拷贝接收缓冲匀
    memcpy(http_info_buf, http_rx_buffer, MAX_PACKET_SIZE);
    uart6_receiver_clear(MAX_PACKET_SIZE);  // 清空接收缓冲匀

    DBG_DEBUG("receiver order_info: %s\r\n", http_info_buf);

    // 解析并保存订區
    int saved = FlashDB_SaveParsedOrders((const char *)http_info_buf);

    if (saved > 0) {
        DBG_INFO("[OrderInfo] Saved %d orders\r\n", saved);
        return saved;  // 返回保存数量
    } else {
        DBG_ERROR("[OrderInfo] GET FAIL!\r\n");
        return -1;  // 失败
    }
}

/**************************************************************************
Function: SaveSystemTime
Input   : time_str - 时间字符串，格式妀{2024-01-01 12:00:00}"
Output  : 返回解析保存结果，表示成功，负值表示失败
函数功能：解析并保存系统时间。从字符串中提取年月日时分秒，存入全局系统时间变量
入口参数：time_str - 时间字符一
返回  值：int - 0:成功 -1:空指针-2:格式错误
**************************************************************************/
int SaveSystemTime(const char *time_str)
{
    if (time_str == NULL)
        return -1;   // 空指针错诀

    int y, M, d, h, m, s;  // 年、月、日、时、分、秒
    // 解析时间字符一
    int ret = sscanf(time_str, "{%d-%d-%d %d:%d:%d}",
                     &y, &M, &d, &h, &m, &s);   

    if (ret != 6) {  // 检查是否成功解析个字段
        DBG_ERROR("时间格式错误: %s\r\n", time_str);
        return -2;   // 格式解析失败
    }

    // 保存到全局系统时间变量
    g_SystemTime.year = y;
    g_SystemTime.month = M;
    g_SystemTime.day = d;
    g_SystemTime.hour = h;
    g_SystemTime.minute = m;
    g_SystemTime.second = s;

    DBG_INFO("时间已保存：%04d-%02d-%02d %02d:%02d:%02d\r\n",
           y, M, d, h, m, s);

    return 0;   // 成功
}

/**************************************************************************
Function: RequestAndSaveSystemTime
Input   : none
Output  : 返回保存结果，表示成功，负值表示失败
函数功能：请求并保存系统时间。发送HTTP请求获取服务器时间，解析后存入全局变量
入口参数：无
返回  值：int - SaveSystemTime的返回值
**************************************************************************/
int RequestAndSaveSystemTime(void)
{
    // 1. 发送查询命什
    USART_4G_SendString("/");

    xSemaphoreTake(xSemaphore4G_RX, pdMS_TO_TICKS(5000));

    vTaskDelay(pdMS_TO_TICKS(200));

    // 2. 拷贝接收的数据
    memcpy(http_info_buf, http_rx_buffer, MAX_PACKET_SIZE);

    // 3. 清空串口缓冲匀
    uart6_receiver_clear(MAX_PACKET_SIZE);

    DBG_DEBUG("Received time data: %s\r\n", http_info_buf);

    // 4. 调用时间解析存储函数
    int ret = SaveSystemTime((const char *)http_info_buf);

    // 5. 错误处理
    if (ret == -1) {
        DBG_ERROR("Error: Null pointer passed!\r\n");
    }
    if(ret == -2) {
        DBG_ERROR("Error: Time format incorrect!\r\n\r\n");
    }

    DBG_INFO("System time updated successfully.\r\n");
    return ret;  // 返回解析结果
}

/**************************************************************************
Function: ParseTimeToMinutes
Input   : t - 时间字符串，格式妀12:30"
Output  : 返回转换后的分钟数，-1表示格式错误
函数功能：将时间字符串转换为分钟数（小时*60+分钟），用于时间比较
入口参数：t - 时间字符一
返回  值：int - 成功:分钟数-1:格式错误
**************************************************************************/
static int ParseTimeToMinutes(const char *t)
{
    int h, m;  // 小时和分针
    if (sscanf(t, "%d:%d", &h, &m) != 2) return -1;  // 解析失败
    return h * 60 + m;  // 返回分钟数
}

/**************************************************************************
Function: CheckDeliveryInfoValid
Input   : now - 当前系统时间指针
          info - 派送信息指针
Output  : 返回检查结果，0表示有效，负值表示具体错误类垀
函数功能：检查是否满足所有派送条件。包括：派送地点非空、处于派送时间区间　
          订单未全部取完、通道状态为1筀
入口参数：now - 当前时间
          info - 派送信息
返回  值：0:有效 -1:地点为空 -2:时间无效 -3:订单已取宀
          -4:通道状态无数-99:参数错误
**************************************************************************/
int CheckDeliveryInfoValid(const SystemTime_t *now, const DeliveryInfo *info)
{
    // 检查参数有效怀
    if (now == NULL || info == NULL) {
        DBG_ERROR("Error: NULL argument!\r\n");
        return -99;  // 参数错误
    }

    /* ============================
       1️⃣  判断派送地点是否为空
       ============================ */
    if (strlen(info->delivery_id) == 0 || strcmp(info->delivery_id, "NULL") == 0) {
        DBG_ERROR("Error: Delivery place is NULL.\r\n");
        return -1;  // 派送地点为空
    }

    /* ============================
       2️⃣ 判断是否处于派送时间区闀
       ============================ */
    int start_h, start_m, end_h, end_m;  // 开始和结束时间的小时、分针

    // 解析开始时闀
    if (sscanf(info->start_time, "%d:%d", &start_h, &start_m) != 2) {
        DBG_ERROR("Error: Invalid start time format!\r\n");
        return -2;  // 开始时间格式错诀
    }
    // 解析结束时间
    if (sscanf(info->end_time, "%d:%d", &end_h, &end_m) != 2) {
        DBG_ERROR("Error: Invalid end time format!\r\n");
        return -2;  // 结束时间格式错误
    }

    // 转换为分钟数
    int now_minutes   = now->hour * 60 + now->minute;
    int start_minutes = start_h * 60 + start_m;
    int end_minutes   = end_h * 60 + end_m;

    // 检查是否在时间区间冀
    if (now_minutes < start_minutes || now_minutes > end_minutes) {
        DBG_ERROR("Error: Not within delivery time! cur=%02d:%02d  range=%s~%s\r\n",
               now->hour, now->minute, info->start_time, info->end_time);
        return -2;  // 不在派送时间区闀
    }

    /* ============================
       3️⃣ 判断订单是否全部取完
       ============================ */
    if (info->picked_count >= info->order_count) {
        DBG_WARN("Error: All orders picked. picked=%d / count=%d\r\n",
               info->picked_count, info->order_count);
        return -3;  // 订单已全部取宀
    }

    /* ============================
       4️⃣ 判断通道状态是否为1
       ============================ */
    if (info->channel_status != 1) {
        DBG_ERROR("Error: Channel status invalid (need=1) now=%d\r\n",
               info->channel_status);
        return -4;  // 通道状态无数
    }

    /* ============================
       5️⃣ 全部通过
       ============================ */
    DBG_INFO("DeliveryInfo PASS: ready to deliver.\r\n");
    return 0;  // 所有条件满超
}

/**************************************************************************
Function: SendOrderProcEvent
Input   : evt - 指向订单处理事件结构体的指针
Output  : 返回发送结果，0表示成功，负值表示失败
函数功能：发送订单处理事件到3条消息队列（显示、LED、语音）。用于多任务间通信
入口参数：evt - 事件结构体指针
返回  值：int - 0:成功 -1:队列未创廀-2:参数错误 -3:全部发送失败
**************************************************************************/
int SendOrderProcEvent(const order_proc_event_t *evt)
{
    /* 1. 队列有效性检柀*/
    if (xOrderProc4G_Show_Queue == NULL ||
        xOrderProc4G_LED_Queue   == NULL ||
        xOrderProc4G_Voice_Queue == NULL) {
        DBG_ERROR("ORDER_QUEUE: queue not created!\r\n");
        return -1;  // 队列未创廀
    }

    /* 2. 参数检柀*/
    if (evt == NULL) {
        DBG_ERROR("ORDER_QUEUE: evt NULL!\r\n");
        return -2;  // 参数错误
    }

    /* 3. 分别发送到 3 条队列（使用非阻塞模式） */
    BaseType_t ret_show  = xQueueSend(xOrderProc4G_Show_Queue, evt, 0);
    BaseType_t ret_led   = xQueueSend(xOrderProc4G_LED_Queue,  evt, 0);
    BaseType_t ret_voice = xQueueSend(xOrderProc4G_Voice_Queue,evt, 0);

    /* 4. 统计成功发送的队列数量 */
    int ok_cnt = (ret_show  == pdTRUE) +
                 (ret_led   == pdTRUE) +
                 (ret_voice == pdTRUE);

    if (ok_cnt == 3) {  // 全部发送成功
        DBG_INFO("ORDER_QUEUE: Event sent to 3 queues, state=%d\r\n", evt->state);
        return 0;  // 成功
    } else {
        DBG_WARN("ORDER_QUEUE: Send partial fail (Show=%d,LED=%d,Voice=%d), state=%d\r\n",
               ret_show, ret_led, ret_voice, evt->state);
        return (ok_cnt == 0) ? -3 : 0;   // 全部失败才返回-3，部分成功也算成功
    }
}

/**************************************************************************
Function: HTTP_task
Input   : pvParameters - FreeRTOS任务参数
Output  : none
函数功能，G通信主任务。管理HTTP通信状态机，处理GET和POST请求，
          更新派送信息、设备配置和订单数据，并控制LED和语音提礀
入口参数：pvParameters - 任务参数
返回  值：无
**************************************************************************/
void HTTP_task(void *pvParameters)
{
    u32 lastWakeTime = getSysTickCnt();  // 获取当前系统时间
    u32 lastSendTime = getSysTickCnt();  // 获取当前系统时间
    
    // 创建3条消息队列，用于不同模块间的通信
    xOrderProc4G_Show_Queue  = xQueueCreate(10, sizeof(order_proc_event_t));// 显示队列
    xOrderProc4G_LED_Queue   = xQueueCreate(10, sizeof(order_proc_event_t));// LED队列
    xOrderProc4G_Voice_Queue = xQueueCreate(10, sizeof(order_proc_event_t));// 语音队列
    
    order_proc_event_t evt;  // 定义事件结构佀
    memset(&evt, 0, sizeof(evt));  // 清空事件结构佀
    
    http_state_t http_state = HTTP_STATE_GET;  // 初始状态为GET
    DeviceMgr_Clear();  // 清空设备管理器
    
    // 初始化标志位
    u8 get_init = 1;        // GET初始化标志
    u8 post_init = 1;       // POST初始化标志
    u8 get_info_init = 1;   // GET信息获取标志
    u8 post_info_init = 1;  // POST信息发送标志
    u8 first_run = 1;       // 首次运行标志
        
    // 重试计数器
    u8 get_retry_count = 0;
    u8 post_retry_count = 0;
    u8 retry_count = 0;

    while (1)  // 主循玀
    {
        vTaskDelayUntil(&lastWakeTime, F2T(RATE_20_HZ));  // 固定频率执行
        
        switch (http_state)  // 状态机处理
        {
            /*==================== 状态1：发通GET 请求 ====================*/
            case HTTP_STATE_GET:
                if(get_init) {  // 第一次进入GET状态
                    // 初始化完进入尝试连接状态
                    evt.state_4G = STATUS_4G_CONNECTING;
                    SendOrderProcEvent(&evt);  // 发送连接中状态

                    HTTP_Module_Init(CONFIG_HTTP_GET_CMD);  // 初始化HTTP GET配置
                    get_init = 0;  // 清除GET初始化标志
                    post_init = 1;  // 设置POST初始化标志
                }
                // 4G云端通信
                if (get_info_init)  // 信息初始化标志位，为1时执行信息初始化
                {
                    get_retry_count++;
                    if(get_retry_count > 5)  // 重试次数过多
                    {
                        // 认定一G传输异常，但仍会重新尝试
                        evt.state_4G = STATUS_4G_DISCONNECTED;
                        evt.state =  ORDER_PROC_DATA_TRANSFER;
                        SendOrderProcEvent(&evt);

                        vTaskDelay(pdMS_TO_TICKS(3000));  // 等待后重诀

                        get_retry_count = 0;  // 重置重试计数
                    }

                    DBG_DEBUG("GET...\r\n");
                    get_info_init = 0;  // 清除标志，避免重复执表
                    
                    // 更新时间
                    if (RequestAndSaveSystemTime() == 0) {
                        DBG_INFO("Time Init OK。\r\n");
                        DBG_INFO("current time:%04d-%02d-%02d %02d:%02d:%02d\r\n",
                                g_SystemTime.year, g_SystemTime.month, g_SystemTime.day,
                                g_SystemTime.hour, g_SystemTime.minute, g_SystemTime.second);
                        retry_count = 0;  // 重置重试计数
                        // 时间传输成功，认一G连接成功
                        evt.state_4G = STATUS_4G_TRANSFER;
                        evt.state = ORDER_PROC_DATA_TRANSFER;

                        SendOrderProcEvent(&evt);  // 发送数据传输中状态
                    } else {
                        if(retry_count >= 5)  // 时间获取失败
                        {
                            retry_count++;
                            DBG_WARN("Time Init Fail! Retry count : %d\r\n", retry_count);
                            evt.state_4G = STATUS_4G_TRANSFER;
                            evt.state =  ORDER_PROC_CLOUD_TIME_FAIL;
                            SendOrderProcEvent(&evt);  // 发送时间获取失败状态
                        }
                        get_info_init = 1;  // 重新尝试获取时间
                        break;
                    }

                    DeliveryInfo info;  // 定义派送信息结构体
                    DBG_DEBUG("\r\n==== Load DeliveryInfo ====\r\n");
                    
                    if (FlashDB_LoadDeliveryInfo(&info) == 0) {  // 本地已有派送信息
                        // 解析开始和结束时间
                        int start_minutes = ParseTimeToMinutes(info.start_time);
                        int end_minutes   = ParseTimeToMinutes(info.end_time);
                        if (start_minutes < 0 || end_minutes < 0) {
                            DBG_ERROR("Delivery time format error! start=%s end=%s\r\n",
                            info.start_time, info.end_time);
                        }
                        /* 2. 获取本地系统时间 */
                        int local_minutes = g_SystemTime.hour * 60 + g_SystemTime.minute;

                        /* 3. 进行比对 */
                        // 不在配送时闀
                        if (local_minutes < start_minutes) {  // 未到派送时闀
                            DBG_INFO("Not yet delivery time! (Current %02d:%02d, Delivery starts %s)\r\n",
                            g_SystemTime.hour, g_SystemTime.minute, info.start_time);
                            DBG_DEBUG("Enter state_4\n");

                            evt.state_4G = STATUS_4G_NORMAL;
                            SendOrderProcEvent(&evt);  // 发通G正常状态

                            http_state = HTTP_STATE_POST;  // 切换到POST状态
                            break;
                        } else if (local_minutes > end_minutes) {  // 派送时间已迀
                            DBG_INFO("Delivery time expired! (Current %02d:%02d, Delivery ends %s)\r\n",
                            g_SystemTime.hour, g_SystemTime.minute, info.end_time);
                            DBG_DEBUG("Enter state_4\n");
                            
                            evt.state_4G = STATUS_4G_NORMAL;
                            SendOrderProcEvent(&evt);  // 发通G正常状态
                            http_state = HTTP_STATE_POST;  // 切换到POST状态
                            break;
                        } else {  // 在派送时间范围内
                            DBG_INFO("Currently within delivery time window.\r\n");
                            // 匹配派送地炀
                            int Delivery_Flag = RequestAndCheckDeliveryID(info.delivery_id);
                            if(Delivery_Flag == 1) {  // ID匹配且通道需要更新
                                DBG_INFO("channel data downloading...\r\n");
                                FlashDB_DeleteAllDeviceProduct();  // 删除旧的通道信息
                                if(!RequestAndSaveDeviceProducts()) {  // 更新通道信息失败
                                    DBG_ERROR("channel data download fail !!!\r\n");
                                    evt.state =  ORDER_PROC_ORDER_DOWNLOAD_FAIL;
                                    evt.state_4G = STATUS_4G_TRANSFER;    
                                    vTaskDelay(pdMS_TO_TICKS(3000));
                                    get_info_init = 1;  // 重新尝试
                                    break;
                                } else {
                                    DBG_INFO("channel data download success !!!\r\n");
                                }
                            } else {
                                DBG_INFO("channel data not need to be updated\r\n");
                            }
                                                        
                            if(Delivery_Flag == 1 || Delivery_Flag == 2) {  // 派送地点与本地相同    
                                DBG_DEBUG("Enter state_5\n");
                                if (info.picked_count >= info.order_count) {  // 订单已取宀
                                    DBG_INFO("All orders picked. picked=%d / count=%d\r\n",
                                    info.picked_count, info.order_count);
                                    
                                    evt.state =  ORDER_PROC_ALL_PICKED ;
                                    evt.state_4G = STATUS_4G_NORMAL;
                                    SendOrderProcEvent( &evt);  // 发送订单已取完状态
                                    
                                    http_state = HTTP_STATE_POST;  // 切换到POST状态
                                    break;
                                } else {  // 未取完，进入派送环节
                                    DBG_INFO("Entering the delivery process\n");
                                    evt.state =  ORDER_PROC_OK ;
                                    evt.state_4G = STATUS_4G_NORMAL;
                                    SendOrderProcEvent(&evt);  // 发送准备就绪状态
                                    OK_4G_flag = 1;  // 设置4G就绪标志
                                    xEventGroupSetBits(xStartupEventGroup, BIT_4G_BUSINESS_READY);
                                }

                                DBG_INFO("Finish\n");
                                break;
                            } else {  // 派送地点与本地不相否
                                evt.state =  ORDER_PROC_DELIVERY_ID_MISMATCH ;
                                evt.state_4G = STATUS_4G_NORMAL;
                                SendOrderProcEvent( &evt);  // 发送地点不匹配状态
                                http_state = HTTP_STATE_POST;  // 切换到POST状态
                                break;
                            }
                        }  
                    } else {  // 不存在本地信息节点3) ------ [首次运行] 
                        DBG_DEBUG("Enter state_3\n");
                        // 请求派送信息
                        if(RequestAndSaveDeliveryPlaces()) {  // 请求派送信息失败
                            evt.state =  ORDER_PROC_UNKNOWN_STATE;
                            evt.state_4G = STATUS_4G_NORMAL;
                            SendOrderProcEvent(&evt);  // 发送未知状态
                            get_info_init = 1;
                            break;
                        }
                        
                        if (FlashDB_LoadDeliveryInfo(&info) == 0) {  // 成功加载派送信息
                            // 检查订单信息表
                            int state_3 = CheckDeliveryInfoValid(&g_SystemTime,&info);
                            switch (state_3)
                            {
                                case 0:  // 所有条件满足，可以派通
                                    DBG_INFO("State_3 OK: Delivery info valid, ready to deliver.\r\n");
                                    FlashDB_DeleteAllDeviceProduct();  // 删除旧的通道信息
                                    if(!RequestAndSaveDeviceProducts()) {  // 更新通道信息失败
                                        DBG_ERROR("channel data download fail !!!\r\n");
                                        evt.state =  ORDER_PROC_ORDER_DOWNLOAD_FAIL;
                                        evt.state_4G = STATUS_4G_TRANSFER;    
                                        vTaskDelay(pdMS_TO_TICKS(3000));
                                        get_info_init = 1;
                                        break;
                                    }
                                    int ret = GetAllOrdersWithPagination();  // 获取所有订區
                                    if(ret > 0) {  // 下载成功
                                        DBG_INFO("Download Info Success!\r\n");
                                        DBG_INFO("Entering the delivery process\n");
                                        evt.state =  ORDER_PROC_ORDER_DOWNLOAD_SUCCESS;
                                        evt.state_4G = STATUS_4G_NORMAL;                                        
                                        OK_4G_flag = 1;    
                                        xEventGroupSetBits(xStartupEventGroup, BIT_4G_BUSINESS_READY);
                                        SendOrderProcEvent(&evt);  // 发送下载成功状态
                                        DBG_INFO("Finish\n");
                                        break;
                                    } else {  // 派送数据下载失败
                                        DBG_ERROR("Delivery orders data download fail !!!\r\n");
                                        evt.state =  ORDER_PROC_ORDER_DOWNLOAD_FAIL;
                                        evt.state_4G = STATUS_4G_TRANSFER;    
                                        vTaskDelay(pdMS_TO_TICKS(3000));
                                        get_info_init = 1;
                                        break;
                                    }
                                    break;

                                case -1:  // 派送地点为空
                                    DBG_ERROR("State_3 ERR: Delivery place is NULL or invalid.\r\n");
                                    evt.state =  ORDER_PROC_PLACE_NULL ;
                                    evt.state_4G = STATUS_4G_TRANSFER;                                                        
                                    vTaskDelay(pdMS_TO_TICKS(10000));
                                    get_info_init = 1;
                                    break;

                                case -2:  // 不在派送时间段 成时间格式错误
                                    DBG_ERROR("State_3 ERR: Not within valid delivery time range.\r\n");
                                    evt.state =  ORDER_PROC_TIME_INVALID ;
                                    evt.state_4G = STATUS_4G_TRANSFER;    
                                    vTaskDelay(pdMS_TO_TICKS(10000));
                                    get_info_init = 1;
                                    break;

                                case -3:  // 订单已经全部取完
                                    DBG_WARN("State_3 WARN: All orders already picked.\r\n");
                                    evt.state =  ORDER_PROC_ALL_PICKED;
                                    evt.state_4G = STATUS_4G_NORMAL;    
                                    vTaskDelay(pdMS_TO_TICKS(10000));
                                    get_info_init = 1;
                                    break;

                                case -4:  // 通道状态不映1
                                    DBG_ERROR("State_3 ERR: Channel status invalid.\r\n");
                                    evt.state =  ORDER_PROC_CHANNEL_INVALID ;
                                    evt.state_4G = STATUS_4G_TRANSFER;    
                                    vTaskDelay(pdMS_TO_TICKS(10000));
                                    get_info_init = 1;
                                    break;

                                case -99:  // 参数为空 成严重错误
                                    DBG_ERROR("State_3 FATAL: NULL pointer or fatal error!\r\n");
                                    evt.state =  ORDER_PROC_FATAL_ERROR ;
                                    evt.state_4G = STATUS_4G_DISCONNECTED;    
                                    vTaskDelay(pdMS_TO_TICKS(10000));
                                    get_info_init = 1;
                                    break;

                                default:  // 未知返回值
                                    DBG_ERROR("State_3 ERR: Unknown return code = %d\r\n", state_3);
                                    evt.state =  ORDER_PROC_UNKNOWN_STATE ;
                                    evt.state_4G = STATUS_4G_DISCONNECTED;    
                                    vTaskDelay(pdMS_TO_TICKS(10000));
                                    get_info_init = 1;
                                    break;
                            }
                            // 统一发送状态至消息队列
                            SendOrderProcEvent( &evt);
                        }
                        vTaskDelay(pdMS_TO_TICKS(1000));
                    }
                } else {
                    evt.state = ORDER_PROC_OK;
                    evt.state_4G = STATUS_4G_NORMAL;
                    if(first_run) {  // 首次运行发送就绪状态
                        SendOrderProcEvent(&evt);
                        first_run = 0;  // 清除首次运行标志
                    }
                }    
                break;
                                            
            /*==================== 状态2：发通POST 请求 ====================*/
            case HTTP_STATE_POST:
                if(post_init) {  // 第一次进入POST状态
                    // 初始化完进入尝试连接状态
                    evt.state_4G = STATUS_4G_CONNECTING;
                    SendOrderProcEvent(&evt);  // 发送连接中状态
                    HTTP_Module_Init(CONFIG_HTTP_POST_CMD);  // 初始化HTTP POST配置
                    get_init = 1;  // 设置GET初始化标志
                    post_init = 0;  // 清除POST初始化标志
                }
                if(post_info_init)  // POST信息发送标志
                {
                    post_info_init = 0;  // 清除标志
                    DBG_DEBUG("POST...\r\n");

                    post_retry_count++;
                    if(post_retry_count > 5)  // 重试次数过多
                    {
                        // 认定一G传输异常，但仍会重新尝试
                        evt.state_4G = STATUS_4G_DISCONNECTED;
                        evt.state =  ORDER_PROC_LOCAL_UPDATE_FAIL;
                        SendOrderProcEvent(&evt);  // 发送本地更新失败状态

                        vTaskDelay(pdMS_TO_TICKS(3000));

                        post_retry_count = 0;  // 重置重试计数
                    }

                    evt.state =  ORDER_PROC_DATA_TRANSFER;
                    evt.state_4G = STATUS_4G_TRANSFER;
                    SendOrderProcEvent(&evt);  // 发送数据传输中状态

                    FlashDB_DeleteAllDeviceProduct();  // 删除设备商品表
                    FlashDB_SendAllOrders();  // 发送所有存储的数据

                    vTaskDelay(pdMS_TO_TICKS(1000));

                    // 如果有接收数据，处理服务器响庀
                    if (strlen((const char *)http_rx_buffer) > 0) {
                        DBG_DEBUG("post: %s\r\n", http_rx_buffer);
                        if (strstr((const char *)http_rx_buffer, "{OK}") != NULL) {  // 收到成功响应
                            DBG_INFO("Cloud update success.\r\n");

                            evt.state_4G = STATUS_4G_NORMAL;
                            evt.state =  ORDER_PROC_LOCAL_UPDATE_SUCCESS ;
                            SendOrderProcEvent( &evt);  // 发送本地更新成功状态

                            FlashDB_ClearAll();  // 成功后删除所有数据
                            http_state = HTTP_STATE_GET;  // 切回GET模式
                            get_info_init = 1;  // 设置GET信息初始化标志
                            DBG_DEBUG("Enter state_6\n");
                        } else {  // 更新失败或收到意外响庀
                            DBG_ERROR("Cloud update failed or unexpected reply.\r\n");

                            evt.state =  ORDER_PROC_LOCAL_UPDATE_FAIL;
                            SendOrderProcEvent( &evt);  // 发送更新失败状态

                            uart6_receiver_clear(MAX_PACKET_SIZE);  // 清空接收缓冲匀
                            post_info_init = 1;  // 重新尝试发通
                            break;
                        }
                    }
                    uart6_receiver_clear(MAX_PACKET_SIZE);  // 清空接收缓冲匀

                    lastSendTime = getSysTickCnt();  // 记录最后发送时闀
                }
                break;

            default:  // 未知状态
                DBG_ERROR("ERROR:NULL http_state!!!\n");
                evt.state_4G = STATUS_4G_DISCONNECTED;
                evt.state =  ORDER_PROC_UNKNOWN_STATE ;
                SendOrderProcEvent( &evt);  // 发送未知状态
                break;
        }
    }
}




