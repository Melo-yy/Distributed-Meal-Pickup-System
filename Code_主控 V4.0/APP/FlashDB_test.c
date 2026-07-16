#include <stdio.h>
#include <string.h>
#include "main.h"
#include <FlashDB_test.h>
#include "debug_config.h"

static struct fdb_kvdb kvdb;
/**************************************************************************
Function: fdb_lock
Input   : db - FlashDB数据库对象指针
Output  : none
函数功能：FlashDB数据库操作锁函数。获取互斥锁，确保多任务环境下对KVDB的互斥访问
入口参数：db - 数据库对象指针（未使用，但符合FlashDB回调函数格式要求）
返回  值：无
**************************************************************************/
void fdb_lock(fdb_db_t db) 
{
    // 获取互斥锁，portMAX_DELAY表示永久等待直到获取到锁
    xSemaphoreTake(xFDB_Mutex, portMAX_DELAY);
}

/**************************************************************************
Function: fdb_unlock
Input   : db - FlashDB数据库对象指针
Output  : none
函数功能：FlashDB数据库操作解锁函数。释放互斥锁，让其他任务可以访问KVDB
入口参数：db - 数据库对象指针（未使用，但符合FlashDB回调函数格式要求）
返回  值：无
**************************************************************************/
static void fdb_unlock(fdb_db_t db)
{
    // 检查互斥锁句柄是否有效，有效则释放锁
    if (xFDB_Mutex) {
        xSemaphoreGive(xFDB_Mutex);
    }
}

/* 默认 KV 节点表 */
static struct fdb_default_kv_node default_kv_table[] = {
    {"def",  "1.0"},  // 默认KV节点：键"def"，值"1.0"
};

/* 默认 KV 描述 */
static struct fdb_default_kv default_kv = {
    .kvs = default_kv_table,  // 指向默认KV节点表
    .num = sizeof(default_kv_table) / sizeof(default_kv_table[0]),  // 计算节点数量
};

/**************************************************************************
Function: FlashDB_Init
Input   : none
Output  : 返回初始化结果，0表示成功，-1表示失败
函数功能：初始化FlashDB键值数据库（KVDB）。设置锁机制、加载默认配置、初始化数据库
入口参数：无
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int FlashDB_Init(void) 
{
    fdb_err_t result;  // 定义FlashDB错误码变量
    
    // 设置KVDB的锁函数，用于多任务保护
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)fdb_lock);
    // 设置KVDB的解锁函数
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, fdb_unlock);
    
    // 初始化KVDB：数据库对象"config"，分区名"fdb_kvdb1"，使用默认KV表
    result = fdb_kvdb_init(&kvdb, "config", "fdb_kvdb1", &default_kv, NULL);
    if (result != FDB_NO_ERR) {
        DBG_ERROR("KVDB 初始化失败! code=%d\r\n", result);  // 打印错误码
        return -1;  // 返回失败
    }
    DBG_INFO("KVDB 初始化完成\r\n");  // 打印成功信息
    return 0;  // 返回成功
}

/**************************************************************************
Function: FlashDB_ClearAll
Input   : none
Output  : 返回执行结果，0表示成功，1表示失败
函数功能：重置KVDB的所有数据。通过擦除整个分区来清空数据库，然后重新初始化
入口参数：无
返回  值：int - 0:成功 1:失败
**************************************************************************/
int FlashDB_ClearAll(void) 
{ 
    const struct fal_partition *part;  // 定义FAL分区指针
    
    // 1. 关闭数据库，释放相关资源
    fdb_kvdb_deinit(&kvdb);
    
    // 2. 找到KVDB对应的FAL分区
    part = fal_partition_find("fdb_kvdb1");
    if (part == NULL) {
        DBG_ERROR("Error: KVDB partition not found!\r\n");  // 分区未找到错误
        return 1;  // 返回失败
    }
    
    // 3. 擦除整个分区，从偏移0开始，擦除整个分区长度
    if (fal_partition_erase(part, 0, part->len) < 0) {
        DBG_ERROR("Error: Erase KVDB partition failed!\r\n");  // 擦除失败错误
        return 1;  // 返回失败
    }
    
    // 4. 重新初始化FAL（Flash抽象层）
    fal_init();
    
    // 5. 重新初始化KVDB，使用相同的配置
    fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);
    
    DBG_INFO("FlashDB KVDB cleared and reinitialized!\r\n");  // 打印成功信息
    return 0;  // 返回成功
}
/**************************************************************************
Function: FlashDB_SaveOrder
Input   : order - 指向要保存的订单结构体的指针
Output  : 返回保存结果，0表示成功，-1表示失败
函数功能：将订单数据存储到FlashDB KV数据库中。使用订单ID和产品ID组合作为键名
入口参数：order - 订单数据指针
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int FlashDB_SaveOrder(order_t *order) 
{
    struct fdb_blob blob;  // 定义FlashDB二进制大对象结构体
    char key[32];          // 定义键名缓冲区

    // 将订单ID和产品ID拼接成唯一键名，格式如："ORD1234561"
    snprintf(key, sizeof(key), "%s%u", order->order_id, order->product_id);
    
    // 创建blob对象，关联order数据和大小
    fdb_blob_make(&blob, order, sizeof(order_t));

    // 将blob作为键值对存储到KVDB中
    fdb_err_t result = fdb_kv_set_blob(&kvdb, key, &blob);
    if (result != FDB_NO_ERR) {
        DBG_ERROR("存储订单失败! code=%d\r\n", result);  // 打印错误码
        return -1;  // 返回失败
    }
    DBG_INFO("订单 %s 已存入 KVDB\r\n", key);  // 打印成功信息
    return 0;  // 返回成功
}

/**************************************************************************
Function: FlashDB_LoadOrder
Input   : order_id - 订单ID字符串指针
          product_id - 产品ID
          out_order - 用于接收订单数据的缓冲区指针
Output  : 返回读取结果，0表示成功，-1表示失败
函数功能：从FlashDB KV数据库中读取指定订单数据
入口参数：order_id - 订单ID
          product_id - 产品ID
          out_order - 输出缓冲区指针
返回  值：int - 0:成功 -1:失败（订单不存在）
**************************************************************************/
int FlashDB_LoadOrder(const char *order_id, uint8_t product_id, order_t *out_order) 
{
    struct fdb_blob blob;  // 定义FlashDB二进制大对象结构体
    char key[32];          // 定义键名缓冲区

    // 将订单ID和产品ID拼接成键名，与保存时保持一致
    snprintf(key, sizeof(key), "%s%u", order_id, product_id);
    
    // 创建blob对象，关联输出缓冲区和大小
    fdb_blob_make(&blob, out_order, sizeof(order_t));

    // 从KVDB获取blob数据，返回实际读取的字节数
    size_t len = fdb_kv_get_blob(&kvdb, key, &blob);
    if (len > 0) {
        DBG_INFO("订单 %s 读取成功\r\n", key);  // 读取成功
        return 0;
    } else {
        DBG_INFO("订单 %s 不存在\r\n", key);    // 读取失败（键不存在）
        return -1;
    }
}

/**************************************************************************
Function: FlashDB_DeleteOrder
Input   : order_id - 订单ID字符串指针
          product_id - 产品ID
Output  : 返回删除结果，0表示成功，-1表示失败
函数功能：从FlashDB KV数据库中删除指定的订单数据
入口参数：order_id - 订单ID
          product_id - 产品ID
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int FlashDB_DeleteOrder(const char *order_id, uint8_t product_id) 
{
    char key[32];           // 定义键名缓冲区
    fdb_err_t result;       // 定义FlashDB错误码变量

    // 根据 order_id 和 product_id 拼接 key
    snprintf(key, sizeof(key), "%s%u", order_id, product_id);

    // 调用 FlashDB 删除接口，删除指定键
    result = fdb_kv_del(&kvdb, key);
    if (result != FDB_NO_ERR) {
        DBG_ERROR("删除订单失败! key=%s, code=%d\r\n", key, result);  // 打印错误信息
        return -1;  // 返回失败
    }

    DBG_INFO("订单 %s 已删除\r\n", key);  // 打印成功信息
    return 0;  // 返回成功
}

/**************************************************************************
Function: FlashDB_UpdateOrderStatus
Input   : order_id - 订单ID字符串指针
          product_id - 产品ID
          new_status - 新的订单状态值
Output  : 返回更新结果，0表示成功，-1表示失败
函数功能：更新指定订单的状态字段。先加载订单，修改状态，再保存回数据库
入口参数：order_id - 订单ID
          product_id - 产品ID
          new_status - 新状态值
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int FlashDB_UpdateOrderStatus(const char *order_id, uint8_t product_id, int new_status) 
{
    order_t order;  // 定义临时订单变量
    
    // 先加载现有订单数据
    if (FlashDB_LoadOrder(order_id, product_id, &order) != 0) {
        return -1;  // 订单不存在，返回失败
    }
    
    // 修改订单状态
    order.status = new_status;
    
    // 保存更新后的订单
    return FlashDB_SaveOrder(&order);
}

/**************************************************************************
Function: FlashDB_PrintOrder
Input   : order - 指向要打印的订单结构体的指针
Output  : none
函数功能：打印单个订单的详细信息，用于调试和显示
入口参数：order - 订单数据指针
返回  值：无
**************************************************************************/
void FlashDB_PrintOrder(const order_t *order) 
{
    DBG_INFO("Order:\r\n");
    DBG_INFO("  pk_id: %d\r\n", order->PK_ID);           // 打印主键ID
    DBG_INFO("  order_id: %d\r\n", order->order_id);     // 打印订单ID
    DBG_INFO("  time_point: %d\r\n", order->time_point); // 打印时间戳
    DBG_INFO("  product_id: %d\r\n", order->product_id); // 打印产品ID
    DBG_INFO("  quantity: %d\r\n", order->quantity);     // 打印数量
    DBG_INFO("  status: %d\r\n", order->status);         // 打印状态
}

#define MAX_ITERATIONS 1024   // 定义最大迭代次数，避免无限循环

/**************************************************************************
Function: FlashDB_PrintAllOrders
Input   : none
Output  : none
函数功能：遍历并打印KVDB中的所有订单数据。使用FlashDB迭代器遍历所有键值对，
          过滤并打印有效的订单信息
入口参数：无
返回  值：无
**************************************************************************/
void FlashDB_PrintAllOrders(void)
{
    struct fdb_kv_iterator iterator;  // 定义KV迭代器
    fdb_kv_t cur_kv;                   // 定义当前KV节点指针
    struct fdb_blob blob;              // 定义blob对象
    order_t order;                      // 定义订单缓冲区
    int iter_count = 0;                 // 初始化迭代计数器
    
    // 初始化迭代器，准备遍历KVDB
    fdb_kv_iterator_init(&kvdb, &iterator);
    
    DBG_INFO("========== 所有订单信息 ==========\r\n");
    
    // 循环遍历所有KV节点
    while (fdb_kv_iterate(&kvdb, &iterator))
    {
        iter_count++;  // 迭代计数加1
        
        // 检查迭代次数是否超过上限，防止意外无限循环
        if (iter_count > MAX_ITERATIONS)
        {
            DBG_WARN("迭代次数超过上限，退出遍历！\r\n");
            break;  // 强制退出循环
        }

        cur_kv = &iterator.curr_kv;  // 获取当前KV节点

        // 创建blob对象，关联order缓冲区
        fdb_blob_make(&blob, &order, sizeof(order_t));

        // 读取当前KV节点的数据到order结构体中
        fdb_blob_read((fdb_db_t)&kvdb, fdb_kv_to_blob(cur_kv, &blob));
        
        // 首先检查数据是否有效（通过判断关键字段是否为空）
        if (strlen(order.PK_ID) == 0 && strlen(order.order_id) == 0)
        {
            DBG_INFO("第%d条数据为空，跳过\r\n", iter_count);  // 空数据跳过
            continue;  // 继续下一次迭代
        }
        
        // 打印有效的订单信息
        DBG_INFO("  PK_ID: %s\r\n", order.PK_ID);                    // 打印主键ID
        DBG_INFO("  order_id: %s\r\n", order.order_id);              // 打印订单ID
        DBG_INFO("  product_id: %u\r\n", order.product_id);          // 打印产品ID
        DBG_INFO("  quantity: %u\r\n", order.quantity);              // 打印数量
        DBG_INFO("  status: %s\r\n", (order.status == 0 ? "未取" : "已取"));  // 打印状态文字
        DBG_INFO("  time_point: %u\r\n", order.time_point);          // 打印时间戳
        DBG_INFO("--------------------------------\r\n");            // 打印分隔线
    }
    
    DBG_INFO("========== 总计 %d 条订单 ==========\r\n", iter_count);  // 打印总计信息
}
/**************************************************************************
Function: FlashDB_SendAllOrders
Input   : none
Output  : none
函数功能：遍历KVDB中的所有订单数据，通过4G模块将订单信息以JSON格式发送出去
          每个订单打包成固定格式的字符串并通过串口发送
入口参数：无
返回  值：无
**************************************************************************/
void FlashDB_SendAllOrders(void)
{
    struct fdb_kv_iterator iterator;  // 定义KV迭代器
    fdb_kv_t cur_kv;                   // 定义当前KV节点指针
    struct fdb_blob blob;              // 定义blob对象
    order_t order;                      // 定义订单缓冲区
    int iter_count = 0;                 // 初始化迭代计数器
    
    // 初始化迭代器，准备遍历KVDB
    fdb_kv_iterator_init(&kvdb, &iterator);
    
    // 循环遍历所有KV节点
    while (fdb_kv_iterate(&kvdb, &iterator))
    {
        iter_count++;  // 迭代计数加1
        
        // 检查迭代次数是否超过上限，防止意外无限循环
        if (iter_count > MAX_ITERATIONS)
        {
            DBG_WARN("迭代次数超过上限，退出遍历！\r\n");
            break;  // 强制退出循环
        }

        cur_kv = &iterator.curr_kv;  // 获取当前KV节点

        // 创建blob对象，关联order缓冲区（栈上分配，避免动态内存分配）
        fdb_blob_make(&blob, &order, sizeof(order_t));

        // 读取当前KV节点的数据到order结构体中
        fdb_blob_read((fdb_db_t)&kvdb, fdb_kv_to_blob(cur_kv, &blob));

        // 将订单信息格式化为字符串，用于发送
        char send_buf[128];  // 定义发送缓冲区
        // 格式化订单数据，格式：{PK_ID,order_id,product_id,quantity,status,time_point}
        snprintf(send_buf, sizeof(send_buf),
                 "{%s,%s,%u,%u,%u,%u}\r\n",
                 order.PK_ID,          // 主键ID
                 order.order_id,        // 订单ID
                 order.product_id,      // 产品ID
                 order.quantity,        // 数量
                 order.status,          // 状态
                 order.time_point);     // 时间戳

        // 通过4G模块串口发送格式化后的订单字符串
        USART_4G_SendString(send_buf);
        // 打印发送的订单信息，用于调试
        DBG_INFO("%s", send_buf);
    }
}

/**************************************************************************
Function: FlashDB_SaveDeviceProduct
Input   : device_id - 设备ID
          product_id - 产品ID
Output  : 返回保存结果，0表示成功，-1表示失败
函数功能：保存设备ID与产品ID的对应关系。用于记录每个设备当前分配的商品通道
入口参数：device_id - 设备ID（16位无符号整数）
          product_id - 产品ID（16位无符号整数）
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int FlashDB_SaveDeviceProduct(uint16_t device_id, uint16_t product_id)
{
    struct fdb_blob blob;  // 定义blob对象
    char key[16];          // 定义键名缓冲区

    // 将设备ID转换为字符串作为键名
    snprintf(key, sizeof(key), "%u", device_id);

    // 创建blob对象，关联product_id数据（保存2字节uint16_t）
    fdb_blob_make(&blob, &product_id, sizeof(product_id));

    // 将设备-产品对应关系存储到KVDB中
    fdb_err_t result = fdb_kv_set_blob(&kvdb, key, &blob);
    if (result != FDB_NO_ERR)
    {
        DBG_ERROR("Save DeviceProduct FAIL! code=%d\r\n", result);  // 打印错误码
        return -1;  // 返回失败
    }

    DBG_INFO("Save OK: dev=%u product=%u\r\n", device_id, product_id);  // 打印成功信息
    return 0;  // 返回成功
}

/**************************************************************************
Function: FlashDB_LoadDeviceProduct
Input   : device_id - 设备ID
          out_product_id - 用于接收产品ID的指针
Output  : 返回读取结果，0表示成功，-1表示失败
函数功能：根据设备ID查询对应的产品ID。用于判断设备是否已分配商品通道
入口参数：device_id - 设备ID（16位无符号整数）
          out_product_id - 输出缓冲区指针，用于返回产品ID
返回  值：int - 0:成功（存在对应关系） -1:失败（不存在或参数错误）
**************************************************************************/
int FlashDB_LoadDeviceProduct(uint16_t device_id, uint16_t *out_product_id)
{
    struct fdb_blob blob;  // 定义blob对象
    char key[16];          // 定义键名缓冲区

    // 检查输出指针是否有效
    if (out_product_id == NULL)
        return -1;  // 无效指针返回失败

    // 将设备ID转换为字符串作为键名
    snprintf(key, sizeof(key), "%u", device_id);

    // 创建blob对象，关联输出缓冲区（读取2字节uint16_t）
    fdb_blob_make(&blob, out_product_id, sizeof(uint16_t));

    // 从KVDB获取blob数据，返回实际读取的字节数
    size_t len = fdb_kv_get_blob(&kvdb, key, &blob);
    if (len == sizeof(uint16_t))  // 成功读取到2字节数据
    {
        DBG_INFO("Load OK: dev=%u product=%u\r\n", device_id, *out_product_id);  // 打印成功信息
        return 0;  // 返回成功
    }
    else
    {
        DBG_WARN("Device[%03X] NOT FOUND!\r\n", device_id);  // 设备不存在警告
        return -1;  // 返回失败
    }
}

/**************************************************************************
Function: FlashDB_DeleteAllDeviceProduct
Input   : none
Output  : 返回删除结果，0表示成功，-1表示失败
函数功能：删除所有设备ID与产品ID的对应关系。遍历从1到16的设备ID，
          检查并删除存在的键值对
入口参数：无
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int FlashDB_DeleteAllDeviceProduct(void)
{
    char device_id[16];      // 定义设备ID字符串缓冲区
    char test_buf[8];        // 定义测试缓冲区，用于检查键是否存在
    struct fdb_blob blob;    // 定义blob对象
    size_t len;              // 定义读取长度变量
    fdb_err_t result;        // 定义FlashDB错误码变量

    // 遍历设备ID从1到16
    for (uint16_t i = 1; i <= 16; i++)
    {
        // 格式化设备ID字符串，格式如："0x001"、"0x002"等
        snprintf(device_id, sizeof(device_id), "0x00%d", i);

        // 创建blob对象，用于检查键是否存在
        fdb_blob_make(&blob, test_buf, sizeof(test_buf));
        // 尝试读取键值，获取数据长度
        len = fdb_kv_get_blob(&kvdb, device_id, &blob);

        // 如果读取长度为0，表示键不存在
        if (len == 0)
        {
            DBG_INFO("Device %s \r\n", device_id);  // 打印设备信息
            break;      // 遇到不存在的键，退出循环
        }

        // 删除当前设备ID对应的键值对
        result = fdb_kv_del(&kvdb, device_id);
        if (result != FDB_NO_ERR)
        {
            DBG_ERROR("error!device=%s, code=%d\r\n", device_id, result);  // 打印错误信息
            return -1;  // 删除失败，返回错误
        }

        DBG_INFO("delete %s\r\n", device_id);  // 打印删除成功信息
    }

    return 0;  // 返回成功
}/**************************************************************************
Function: SaveDeliveryInfo
Input   : info - 指向派送信息结构体的指针
Output  : 返回保存结果，0表示成功，-1表示失败
函数功能：保存派送信息（如派送时间、地点等）到FlashDB KV数据库中。
          使用固定键名"delivery_info"存储，每次保存会覆盖原有数据
入口参数：info - 派送信息数据指针
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int SaveDeliveryInfo(const DeliveryInfo *info)
{
    struct fdb_blob blob;  // 定义FlashDB二进制大对象结构体
    
    // 创建blob对象，关联派送信息数据和大小
    fdb_blob_make(&blob, info, sizeof(DeliveryInfo));
    
    // 将blob作为键值对存储到KVDB中，使用固定键名"delivery_info"
    fdb_err_t result = fdb_kv_set_blob(&kvdb, "delivery_info", &blob);
    if (result != FDB_NO_ERR) {
        DBG_ERROR("存储失败! code=%d\r\n", result);  // 打印错误码
        return -1;  // 返回失败
    }
    DBG_INFO("DeliveryInfo 已存入 KVDB\r\n");  // 打印成功信息
    return 0;  // 返回成功
}

/**************************************************************************
Function: FlashDB_LoadDeliveryInfo
Input   : out_info - 用于接收派送信息的缓冲区指针
Output  : 返回读取结果，0表示成功，-1表示失败
函数功能：从FlashDB KV数据库中读取当前保存的派送信息
入口参数：out_info - 输出缓冲区指针，用于返回派送信息数据
返回  值：int - 0:成功 -1:失败（数据不存在）
**************************************************************************/
int FlashDB_LoadDeliveryInfo(DeliveryInfo *out_info)
{
    struct fdb_blob blob;  // 定义FlashDB二进制大对象结构体
    
    // 创建blob对象，关联输出缓冲区和大小
    fdb_blob_make(&blob, out_info, sizeof(DeliveryInfo));

    // 从KVDB获取blob数据，使用固定键名"delivery_info"
    size_t len = fdb_kv_get_blob(&kvdb, "delivery_info", &blob);

    // 判断是否成功读取到"delivery_info"键值对
    if (len > 0) {
        DBG_INFO("DeliveryInfo 读取成功 (size=%u)\r\n", (unsigned)len);  // 打印成功信息和数据大小
        return 0;  // 返回成功
    } else {
        DBG_WARN("DeliveryInfo 不存在\r\n");  // 打印警告信息
        return -1;  // 返回失败
    }
}

/**************************************************************************
Function: FlashDB_TestLoadDeliveryInfo
Input   : none
Output  : 返回测试结果，0表示成功，-1表示失败
函数功能：测试加载并打印派送信息的所有字段。用于验证派送信息的读取功能
入口参数：无
返回  值：int - 0:成功 -1:失败
**************************************************************************/
int FlashDB_TestLoadDeliveryInfo(void)
{
    DeliveryInfo info;  // 定义派送信息变量

    DBG_INFO("\r\n==== Load DeliveryInfo ====\r\n");  // 打印测试开始标记

    // 尝试加载派送信息
    if (FlashDB_LoadDeliveryInfo(&info) == 0)
    {
        // 打印所有字段信息（英文输出）
        DBG_INFO("PK_ID: %s\r\n", info.PK_ID);                    // 打印主键ID
        DBG_INFO("Delivery ID: %s\r\n", info.delivery_id);        // 打印派送ID
        DBG_INFO("Delivery Date: %s\r\n", info.delivery_date);    // 打印派送日期
        DBG_INFO("Delivery Time: %s\r\n", info.delivery_time);    // 打印派送时间
        DBG_INFO("Start Time: %s\r\n", info.start_time);          // 打印开始时间
        DBG_INFO("End Time: %s\r\n", info.end_time);              // 打印结束时间
        DBG_INFO("Order Count: %u\r\n", info.order_count);        // 打印订单总数
        DBG_INFO("Picked Count: %u\r\n", info.picked_count);      // 打印已取件数量
        DBG_INFO("Order Status: %u\r\n", info.order_status);      // 打印订单状态
        DBG_INFO("Status Sync Time: %s\r\n", info.status_sync_time); // 打印状态同步时间
        DBG_INFO("Channel Status: %u\r\n", info.channel_status);  // 打印通道状态
        DBG_INFO("Device Time: %s\r\n", info.device_time);        // 打印设备时间

        DBG_INFO("==== Load DeliveryInfo Finished ====\r\n\r\n");  // 打印测试结束标记
        return 0;  // 返回成功
    }
    else
    {
        DBG_ERROR("Load DeliveryInfo Failed!\r\n");  // 打印错误信息
        return -1;  // 返回失败
    }
}

/**************************************************************************
Function: FlashDB_Test
Input   : none
Output  : none
函数功能：FlashDB订单功能综合测试函数。依次测试订单的保存、读取、
          状态更新、删除等所有功能，用于验证数据库操作的正确性
入口参数：无
返回  值：无
**************************************************************************/
void FlashDB_Test(void) 
{
    order_t order;          // 定义原始订单变量
    order_t read_order;     // 定义读取订单变量

    /* 1. 构造测试订单 */
    memset(&order, 0, sizeof(order));  // 清空订单结构体

    // 填充测试数据
    strcpy(order.PK_ID, "PK001");           // 设置主键ID
    strcpy(order.order_id, "A123");         // 设置订单ID（4字符 + \0 = 5字节刚好）
    order.product_id = 2;                    // 设置产品ID
    order.quantity = 5;                      // 设置数量
    order.status = 0;                        // 设置状态：0表示未取
    order.time_point = 12345678;              // 设置时间戳

    DBG_INFO("\r\n===== FlashDB 订单测试开始 =====\r\n");  // 打印测试开始标记

    /* 2. 保存订单 */
    DBG_INFO("[1] 保存订单...\r\n");
    if (FlashDB_SaveOrder(&order) == 0) {
        DBG_INFO("保存成功\r\n");  // 保存成功
    }

    /* 3. 读取订单 */
    DBG_INFO("[2] 读取订单...\r\n");
    if (FlashDB_LoadOrder(order.order_id, order.product_id, &read_order) == 0) {
        // 打印读取到的订单信息
        DBG_INFO("读取成功：order_id=%s product_id=%d quantity=%d status=%d time=%lu\r\n",
               read_order.order_id,
               read_order.product_id,
               read_order.quantity,
               read_order.status,
               read_order.time_point);
    }

    /* 4. 修改订单状态 */
    DBG_INFO("[3] 修改订单状态为已取(1)...\r\n");
    if (FlashDB_UpdateOrderStatus(order.order_id, order.product_id, 1) == 0) {
        DBG_INFO("更新成功\r\n");  // 状态更新成功
    }

    /* 5. 再读取一次确认状态变化 */
    DBG_INFO("[4] 再次读取订单...\r\n");
    if (FlashDB_LoadOrder(order.order_id, order.product_id, &read_order) == 0) {
        // 确认状态已更新
        DBG_INFO("状态更新确认：status=%d\r\n", read_order.status);
    }

    /* 6. 删除订单 */
    DBG_INFO("[5] 删除订单...\r\n");
    if (FlashDB_DeleteOrder(order.order_id, order.product_id) == 0) {
        DBG_INFO("删除成功\r\n");  // 删除成功
    }

    /* 7. 检查是否真的删掉 */
    DBG_INFO("[6] 删除后再次读取...\r\n");
    if (FlashDB_LoadOrder(order.order_id, order.product_id, &read_order) != 0) {
        // 读取失败说明订单已被删除，符合预期
        DBG_INFO("读取失败，订单已被删除（正常）\r\n");
    }

    DBG_INFO("===== FlashDB 订单测试结束 =====\r\n");  // 打印测试结束标记
}