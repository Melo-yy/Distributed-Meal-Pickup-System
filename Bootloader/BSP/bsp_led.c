#include "bsp_led.h"
#include "bsp_delay.h"

void bsp_LED_Host_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIO clocks */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOE, ENABLE);

    /* Configure RGB LED pins: PB0(Red), PB1(Green), PE7(Blue) */
    GPIO_InitStructure.GPIO_Pin = LED_RED_PIN | LED_GREEN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LED_RED_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = LED_BLUE_PIN;
    GPIO_Init(LED_BLUE_PORT, &GPIO_InitStructure);

    /* All LEDs off */
    LED_OFF_ALL();
}
