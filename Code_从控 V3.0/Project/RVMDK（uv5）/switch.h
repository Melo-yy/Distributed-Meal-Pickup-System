#ifndef __SWITCH_H
#define __SWITCH_H



void Init_switchdata(void);
void SWITCH_Init();
void Key_Init(void);
uint8_t Key_Scan(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin);
void Switch_flexible_up_egg();
void Switch_flexible_down_egg();
void Switch_flexible_stop_egg();
void IF_Switch_push_egg_start();
void IF_Switch_push_egg_end();
void IF_Switch_start_push_plate_();
void IF_Switch_push_plate_end();
void IF_Switch_push_plate_start();



#endif
