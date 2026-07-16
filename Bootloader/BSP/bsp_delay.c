#include "bsp_delay.h"
#include "bsp_sys.h"

/* Simple polling delay using SysTick (no OS, no interrupts) */
static uint8_t fac_us = 0;

/* Initialize delay using SysTick in polling mode */
void delay_init(void)
{
    /* Configure SysTick: HCLK/8 as clock source for simpler calculation */
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
    fac_us = (SystemCoreClock / 8) / 1000000;  /* us multiplier */
}

/* Microsecond delay, polling mode */
void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;

    if (nus == 0) return;

    ticks = nus * fac_us;
    told = SysTick->VAL;
    while (1) {
        tnow = SysTick->VAL;
        if (tnow != told) {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if (tcnt >= ticks)
                break;
        }
    }
}

/* Millisecond delay */
void delay_ms(uint32_t nms)
{
    uint32_t i;
    for (i = 0; i < nms; i++) {
        delay_us(1000);
    }
}
