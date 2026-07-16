#ifndef __MAIN_H_
#define __MAIN_H_

#include "FreeRTOS.h"     // 必须
#include "task.h"         // 必须，TaskHandle_t 定义在这里

#include "stm32f10x.h"                  // Device header
#include "string.h" 
#include "stdarg.h" 
#include "./delay/delay.h"

#include "freertos.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#include "LED.h"
#include "OLED.h"
#include "task.h"
#include "usart.h"

#include "bsp_show.h"
#include "bsp_led.h"
#include "bsp_sys.h"
#include "bsp_can.h"
#include "bsp_scan.h"
#include "bsp_voice.h"


#include "app_show.h"
#include "app_led.h"
#include "app_system.h"
#include "app_can.h"
#include "app_can_common.h"
#include "app_scan.h"
#include "app_voice.h"






#endif

