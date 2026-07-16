#ifndef __MAIN_H
#define __MAIN_H

// Refer to all header files you need
//引用所有需要用到的头文件
#include "FreeRTOSConfig.h"
//FreeRTOS相关头文件
//FreeRTOS related header files
#include "FreeRTOS.h"
#include "stm32f4xx.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"
//The associated header file for the peripheral
//外设的相关头文件

#include "bsp_delay.h"
#include "bsp_usart.h"
#include "bsp_led.h"
#include "bsp_show.h"
#include "bsp_key.h"
#include "bsp_sys.h"
#include "bsp_can.h"
#include "bsp_scan.h"
#include "bsp_voice.h"
#include "bsp_4g.h"

#include "app_show.h"
#include "app_led.h"
#include "app_system.h"
#include "app_can.h"
#include "app_can_common.h"
#include "app_scan.h"
#include "app_voice.h"
#include "app_4g.h"
//OTA
#include "ota_meta.h"

//FlashDB的相关头文件
#include <flashdb.h>
#include <FlashDB_test.h>

extern SemaphoreHandle_t xFDB_Mutex;
extern SemaphoreHandle_t xSemaphoreScan;    /* 扫码帧到达信号量（USART2 ISR 释放，scan_task 阻塞等待） */

/* 启动同步事件组：各子系统初始化完成时置位，业务任务阻塞等待依赖就绪 */
extern EventGroupHandle_t xStartupEventGroup;
#define BIT_CAN_READY          (1 << 0)   /* CAN 硬件初始化完成（未使用） */
#define BIT_4G_READY           (1 << 1)   /* 4G 硬件 USART6 就绪（systemInit 中设置） */
#define BIT_FLASHDB_READY      (1 << 2)   /* FlashDB 初始化完成 */
#define BIT_4G_BUSINESS_READY  (1 << 3)   /* 4G 业务就绪：配货单/订单下载完成（HTTP_task 中设置） */
#define BIT_ALL_SYSTEMS        (BIT_CAN_READY | BIT_4G_READY | BIT_FLASHDB_READY | BIT_4G_BUSINESS_READY)


/***Macros define***/ /***宏定义***/


//C library function related header file
//C库函数的相关头文件
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stdarg.h"
#endif
