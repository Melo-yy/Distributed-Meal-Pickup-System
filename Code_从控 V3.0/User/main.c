#include "stm32f10x.h"                  // Device header
#include "main.h"

/*   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER*/
TaskHandle_t StartTask_Handler;
TaskHandle_t Show_Task_Hadnler;
TaskHandle_t LED_Task_Handler;
TaskHandle_t CAN_Task_Handeler;
TaskHandle_t SCAN_Task_Handeler;
TaskHandle_t VOICE_Task_Handeler;
/*   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER   TASKHANDLER*/

//Task priority    //任务优先级
#define START_TASK_PRIO	1

//Task stack size //任务堆栈大小	
#define START_STK_SIZE 	256  
void start_task(void *pvParameters);

int main(void)
{
		System_Init();
		xTaskCreate((TaskFunction_t )start_task,            //Task function   //任务函数
							(const char*    )"start_task",          //Task name       //任务名称
							(uint16_t       )START_STK_SIZE,        //Task stack size //任务堆栈大小
							(void*          )NULL,                  //Arguments passed to the task function //传递给任务函数的参数
							(UBaseType_t    )START_TASK_PRIO,       //Task priority   //任务优先级
							(TaskHandle_t*  )&StartTask_Handler);   //Task handle     //任务句柄   
	vTaskStartScheduler();
	
			
}

void start_task(void *pvParameters)
{
    taskENTER_CRITICAL(); //Enter the critical area //进入临界区	
    //Create the task //创建任务  
		xTaskCreate(show_task,     "show_task",     SHOW_STK_SIZE,     NULL, SHOW_TASK_PRIO,     &Show_Task_Hadnler); //The OLED display displays tasks //显示屏显示任务
    xTaskCreate(led_task,      "led_task",      LED_STK_SIZE,      NULL, LED_TASK_PRIO,      &LED_Task_Handler);	//LED light flashing task //LED灯闪烁任务
    xTaskCreate(can_task,      "can_task",      CAN_STK_SIZE,      NULL, CAN_TASK_PRIO,      &CAN_Task_Handeler);	
	  xTaskCreate(scan_task,     "scan_task",     SCAN_STK_SIZE,     NULL, SCAN_TASK_PRIO,     &SCAN_Task_Handeler);	
    xTaskCreate(voice_task,    "voice_task",    VOICE_STK_SIZE,    NULL, VOICE_TASK_PRIO,    &VOICE_Task_Handeler);	
	  // 栈监控任务
//    xTaskCreate(stack_monitor_task, "stack_mon", STACK_MONITOR_STK_SIZE, NULL, STACK_MONITOR_TASK_PRIO, NULL);
//		printf("[系统启动] 创建所有任务后，可用堆: %lu 字节\n", xPortGetFreeHeapSize());
    vTaskDelete(StartTask_Handler); //Delete the start task //删除开始任务
    taskEXIT_CRITICAL();            //Exit the critical section//退出临界区
}


