#ifndef _DELAY_H_
#define _DELAY_H_

#include "main.h"

void Delay_Init(void);
void delay_init(void);
void Delay_us(unsigned short us);
void Delay_ms(unsigned short ms);
void DelayMs(unsigned short ms);


void delay_init_f103(void);
void Delay_Ms_Sync(uint32_t ms);


u32 getSysTickCnt(void);
#endif
