/***********************************************
版本：V1.0
修改时间：2025-8-17

Version: V1.0               
Update：2025-8-17
***********************************************/

#include "app_system.h"

void System_Init(void)
{
		// 中断优先级分组设置
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
		Delay_Init();
		Delay_Ms_Sync(3000);
	
		//LED初始化
		bsp_LED_Init();
	
		Init_Usart();   //初始化3个串口（包含了DEBUG(语音)、显示、扫码）
			
		// 初始化BSP层CAN
    	bsp_can_Init();
		// 配置从机过滤器：接收与从机id相同的响应
//		bsp_can_Filter_Config(MY_SLAVE_ID, 0x7FF);
	
	
}


/**
 * 测试阶段使用
 * 功能：每2秒打印所有任务的栈水位线（历史最低剩余）
 */
void stack_monitor_task(void *pvParameters)
{
    // 任务句柄数组（按你的实际任务数量和顺序添加）
    // 如果你不需要某个任务的句柄，可以放 NULL，它会被跳过
    const struct {
        TaskHandle_t *handle;
        const char   *name;
    } tasks[] = {
        {&Show_Task_Hadnler,     "show_task"},
        {&LED_Task_Handler,      "led_task"},
        {&CAN_Task_Handeler,     "can_task"},
        {&SCAN_Task_Handeler,    "scan_task"},
        {&VOICE_Task_Handeler,   "voice_task"},
				{&StartTask_Handler,		 "start_task"	},
        {NULL,                   NULL}               // 结束标记
    };

    // 打印表头（第一次执行时）
    printf("\r\n=== Stack High Water Mark Monitor (every 2s) ===\r\n");
    printf("Task Name          | Remaining (words) | Remaining (bytes)\r\n");
    printf("-------------------+--------------------+------------------\r\n");

    while (1)
    {
        for (int i = 0; tasks[i].handle != NULL; i++)
        {
            if (*(tasks[i].handle) != NULL)  // 任务句柄有效（start_task 已删除会为 NULL）
            {
                UBaseType_t waterMark = uxTaskGetStackHighWaterMark(*(tasks[i].handle));
                printf("%-18s | %18lu | %17lu\r\n",
                       tasks[i].name,
                       (uint32_t)waterMark,
                       (uint32_t)(waterMark * 4));  // 1 word = 4 bytes
            }
        }
        printf("---------------------------------------------------\r\n");
				printf("Free heap: %d bytes\r\n", xPortGetFreeHeapSize());
        vTaskDelay(pdMS_TO_TICKS(2000));  // 每2秒打印一次
    }
}
