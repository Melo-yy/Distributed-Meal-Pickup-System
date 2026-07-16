#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "bsp_sys.h"

/* RGB LED on PB0(Red), PB1(Green), PE7(Blue) */
#define LED_RED_PORT      GPIOB
#define LED_RED_PIN       GPIO_Pin_0
#define LED_GREEN_PORT    GPIOB
#define LED_GREEN_PIN     GPIO_Pin_1
#define LED_BLUE_PORT     GPIOE
#define LED_BLUE_PIN      GPIO_Pin_7

#define LED_RED_ON()      GPIO_SetBits(LED_RED_PORT, LED_RED_PIN)
#define LED_RED_OFF()     GPIO_ResetBits(LED_RED_PORT, LED_RED_PIN)
#define LED_GREEN_ON()    GPIO_SetBits(LED_GREEN_PORT, LED_GREEN_PIN)
#define LED_GREEN_OFF()   GPIO_ResetBits(LED_GREEN_PORT, LED_GREEN_PIN)
#define LED_BLUE_ON()     GPIO_SetBits(LED_BLUE_PORT, LED_BLUE_PIN)
#define LED_BLUE_OFF()    GPIO_ResetBits(LED_BLUE_PORT, LED_BLUE_PIN)

#define LED_BOOT_INDICATE()  do { LED_RED_ON(); delay_ms(100); LED_RED_OFF(); \
                                  LED_GREEN_ON(); delay_ms(100); LED_GREEN_OFF(); \
                                  LED_BLUE_ON(); delay_ms(100); LED_BLUE_OFF(); } while(0)
#define LED_ERROR()           LED_RED_ON()
#define LED_SUCCESS()         LED_GREEN_ON()
#define LED_OFF_ALL()         do { LED_RED_OFF(); LED_GREEN_OFF(); LED_BLUE_OFF(); } while(0)

void bsp_LED_Host_Init(void);

#endif
