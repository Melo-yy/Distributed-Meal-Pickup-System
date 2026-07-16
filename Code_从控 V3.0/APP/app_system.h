#ifndef __APP_SYSTEM_H
#define __APP_SYSTEM_H

#include "main.h"

// 在头文件中添加（或其他地方）
#define STACK_MONITOR_TASK_PRIO    1    // 最低优先级即可
#define STACK_MONITOR_STK_SIZE     192  // 监控任务本身栈很小就够

extern TaskHandle_t StartTask_Handler;
extern TaskHandle_t Show_Task_Hadnler;
extern TaskHandle_t LED_Task_Handler;
extern TaskHandle_t CAN_Task_Handeler;
extern TaskHandle_t SCAN_Task_Handeler;
extern TaskHandle_t VOICE_Task_Handeler;
extern TaskHandle_t StartTask_Handler;

void System_Init(void);
void stack_monitor_task(void *pvParameters);

#endif
