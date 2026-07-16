/***********************************************
Project:  High-concurrency meal-picking traceability device
Data   :  2025-8-19
Developer: Melo & 大水鱼
***********************************************/
#include "main.h"

//Task priority
#define START_TASK_PRIO	1

//Task stack size
#define START_STK_SIZE 	256

//Task handle
TaskHandle_t StartTask_Handler;
TaskHandle_t Balance_task_Handler;
TaskHandle_t Show_Task_Hadnler;
TaskHandle_t LED_Task_Handler;
TaskHandle_t CAN_Task_Handeler;
TaskHandle_t SCAN_Task_Handeler;
TaskHandle_t VOICE_Task_Handeler;
TaskHandle_t HTTP_Task_Handeler;
TaskHandle_t OTA_Task_Handler;

//Task function
void start_task(void *pvParameters);

SemaphoreHandle_t xFDB_Mutex;
SemaphoreHandle_t xSemaphore4G_RX;      /* 4G 模块数据到达信号量（ISR 释放，ota_http_get 阻塞等待） */
SemaphoreHandle_t xSemaphoreScan;       /* 扫码帧到达信号量（USART2 ISR 释放，scan_task 阻塞等待） */
EventGroupHandle_t xStartupEventGroup;  /* 启动同步事件组 */

//Main function
int main(void)
{
	systemInit();

	xTaskCreate((TaskFunction_t )start_task,
				(const char*    )"start_task",
				(uint16_t       )START_STK_SIZE,
				(void*          )NULL,
				(UBaseType_t    )START_TASK_PRIO,
				(TaskHandle_t*  )&StartTask_Handler);
	vTaskStartScheduler();
}

//Start task function
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();
	xFDB_Mutex = xSemaphoreCreateMutex();
	xSemaphore4G_RX = xSemaphoreCreateBinary();
	xSemaphoreScan = xSemaphoreCreateBinary();
	xStartupEventGroup = xEventGroupCreate();

    /* Init FlashDB */
    if (FlashDB_Init() != 0) {
        DBG_DEBUG(" FlashDB init fail!\r\n");
        return;
    }
    DBG_DEBUG(" FlashDB init success!\r\n");
	FlashDB_ClearAll();
	xEventGroupSetBits(xStartupEventGroup, BIT_FLASHDB_READY);

    //Create tasks
	xTaskCreate(show_task,     "show_task",     SHOW_STK_SIZE,     NULL, SHOW_TASK_PRIO,     Show_Task_Hadnler);
    xTaskCreate(led_task,      "led_task",      LED_STK_SIZE,      NULL, LED_TASK_PRIO,      LED_Task_Handler);
    xTaskCreate(can_task,      "can_task",      CAN_STK_SIZE,      NULL, CAN_TASK_PRIO,      CAN_Task_Handeler);
	xTaskCreate(scan_task,     "scan_task",     SCAN_STK_SIZE,     NULL, SCAN_TASK_PRIO,     SCAN_Task_Handeler);
    xTaskCreate(voice_task,    "voice_task",    VOICE_STK_SIZE,    NULL, VOICE_TASK_PRIO,    VOICE_Task_Handeler);
	xTaskCreate(HTTP_task,     "HTTP_task",     HTTP_STK_SIZE,     NULL, HTTP_TASK_PRIO,     HTTP_Task_Handeler);
	xTaskCreate(OTA_Task,      "ota_task",      OTA_TASK_STK_SIZE, NULL, OTA_TASK_PRIO,      &OTA_Task_Handler);

    vTaskDelete(StartTask_Handler);
    taskEXIT_CRITICAL();
}
