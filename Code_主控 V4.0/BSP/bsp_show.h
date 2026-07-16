#ifndef __BSP_SHOW_H
#define __BSP_SHOW_H			  	

#include "bsp_sys.h"
#include "bsp_usart.h"
#include "bsp_delay.h"


void bsp_show_init(void);
void bsp_show_switch(const char* fmt, ...);

#endif  
	 
