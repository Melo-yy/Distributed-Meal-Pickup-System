/***********************************************
版本：V1.0
修改时间：2025-8-17

Version: V1.0               
Update：2025-8-17
***********************************************/

#include "app_system.h"
#include "ota_meta.h"
#include "FreeRTOS.h"
#include "event_groups.h"

/**************************************************************************
Function: systemInit
Input   : none
Output  : none
函数功能：系统底层硬件资源初始化汇总。配置 NVIC 优先级、延时、串口、CAN、LED 及存储管理
入口参数：无
返回  值：无
**************************************************************************/
void systemInit(void)
{
	// 中断优先级分组设置
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	// 延时函数初始化 
	delay_init(168);
	delay_ms(3000);

	// DEBUG串口初始化
	bsp_usart_init(DEBUG_USART,115200);
	// DEBUG互斥锁初始化
	bsp_Debug_Mutex_init();
	
	bsp_USART6_4g_Init(115200);
	/* 4G 就绪 → 通知 OTA 任务可以开始 */
	if (xStartupEventGroup != NULL) {
		xEventGroupSetBits((EventGroupHandle_t)xStartupEventGroup, BIT_4G_READY);
	}

	// 扫描设备初始化
	bsp_scan_init();

	// 初始化BSP层CAN	
  	bsp_can_Init();

	//LED初始化
	bsp_LED_Host_Init();
	
	// 语音模块串口初始化
	bsp_voice_init();
	
	// 串口屏初始化
	bsp_show_init();
	
	// device_id存储初始化
	DeviceMgr_Init();
	
	// OTA_Init()：读取 Slot1 元数据，确认启动分区（A/B）和当前固件版本
	OTA_Init();
	
}
