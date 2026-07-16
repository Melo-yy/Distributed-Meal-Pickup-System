#ifndef __FLASHDB_TEST_H
#define __FLASHDB_TEST_H

#include "main.h"

typedef struct {
		char PK_ID[8];
		char order_id[10];
		uint8_t product_id;
    uint8_t quantity;
    uint8_t  status;   // 0=未取，1=已取
		uint32_t time_point;
} order_t;

//typedef struct {
//		char PK_ID[8];
//    char device_id[8];   // 装置ID
//    char  product_id[8];  // 商品ID
//} device_product_t;

typedef struct {
    char PK_ID[10];                // 主键
    char delivery_id[10];        // 派送地点名称
    char delivery_date[12];        // 派送日期 YYYY-MM-DD
    char delivery_time[6];        // 派送时间 
    char start_time[6];           // 派送开始时间
    char end_time[6];             // 派送结束时间
    uint16_t order_count;          // 订单数
    uint16_t picked_count;         // 已取订单数
    uint8_t order_status;         // 订单状态
    char status_sync_time[10];     // 订单状态同步时间
    uint8_t channel_status;       // 通道状态
    char device_time[10];          // 通道设置时间
} DeliveryInfo;

/* KVDB 对象 */
static struct fdb_kvdb kvdb;
int FlashDB_Init(void);
int FlashDB_ClearAll(void);
int FlashDB_SaveOrder(order_t *order);
int FlashDB_LoadOrder(const char *order_id, uint8_t product_id, order_t *out_order);
int FlashDB_DeleteOrder(const char *order_id, uint8_t product_id);
int FlashDB_UpdateOrderStatus(const char *order_id, uint8_t product_id, int new_status);
void FlashDB_PrintOrder(const order_t *order);
void FlashDB_PrintAllOrders(void);
void FlashDB_SendAllOrders(void);//存在问题会发送所有的数据库数据
int SaveDeliveryInfo(const DeliveryInfo *info);
int FlashDB_SaveDeviceProduct(uint16_t device_id, uint16_t product_list);
int FlashDB_LoadDeviceProduct(uint16_t device_id, uint16_t *out_product_list);
int FlashDB_DeleteAllDeviceProduct(void);



void FlashDB_Test(void);
void test_device_kv(void);
int FlashDB_LoadDeliveryInfo(DeliveryInfo *out_info);
int FlashDB_TestLoadDeliveryInfo(void);



#endif
