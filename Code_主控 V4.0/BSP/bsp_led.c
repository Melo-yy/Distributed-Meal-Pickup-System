#include "bsp_led.h"

// 闪烁控制结构
typedef struct {
    uint8_t is_blinking;
    uint32_t interval_ms;
    uint32_t last_toggle_time;
    uint8_t current_state; // 0: off, 1: on
    uint8_t blink_red;
    uint8_t blink_green;
    uint8_t blink_blue;
} led_blink_ctrl_t;

static led_blink_ctrl_t blink_controls[HOST_LED_MAX];

void bsp_LED_Host_Init(void)
{    
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 使能Host装置LED时钟
    RCC_AHB1PeriphClockCmd(HOST_4G_GPIO_CLK, ENABLE);

    // 2. 配置Host 4G指示灯 (红色和绿色)
    GPIO_InitStructure.GPIO_Pin = HOST_4G_RED_PIN | HOST_4G_GREEN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(HOST_4G_GPIO_PORT, &GPIO_InitStructure);
    
    // 3. 配置Host 4G指示灯 (蓝色)
    GPIO_InitStructure.GPIO_Pin = HOST_4G_BLUE_PIN;
    GPIO_Init(HOST_4G_BLUE_PIN_PORT, &GPIO_InitStructure);

    // 4. 初始化所有LED为关闭状态
    bsp_LED_Host_SetColor(HOST_LED_4G, 0, 0, 0);
    
    // 5. 初始化闪烁控制结构
    memset(blink_controls, 0, sizeof(blink_controls));
    
}

void bsp_LED_Host_SetColor(Host_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue) 
{
    // 停止闪烁
    if (led_type < HOST_LED_MAX) {
        blink_controls[led_type].is_blinking = 0;
    }
    
    switch (led_type) {
        case HOST_LED_4G:
            red   ? HOST_4G_RED_ON()   : HOST_4G_RED_OFF();
            green ? HOST_4G_GREEN_ON() : HOST_4G_GREEN_OFF();
            blue  ? HOST_4G_BLUE_ON()  : HOST_4G_BLUE_OFF();
            break;
        default:
            break;
    }
}

void bsp_LED_Host_SetSolidColor(Host_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue)
{
    bsp_LED_Host_SetColor(led_type, red, green, blue);
}

void bsp_LED_Host_SetBlinkColor(Host_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue, uint32_t interval_ms)
{
    if (led_type >= HOST_LED_MAX) return;
    
    blink_controls[led_type].is_blinking = 1;
    blink_controls[led_type].interval_ms = interval_ms;
    blink_controls[led_type].last_toggle_time = getSysTickCnt();
    blink_controls[led_type].current_state = 1;
    blink_controls[led_type].blink_red = red;
    blink_controls[led_type].blink_green = green;
    blink_controls[led_type].blink_blue = blue;
    
    // 初始状态为亮
    bsp_LED_Host_SetColor(led_type, red, green, blue);
}

void bsp_LED_Host_UpdateBlink(void)
{
    uint32_t current_time = getSysTickCnt();
    
    for (int i = 0; i < HOST_LED_MAX; i++) {
        if (blink_controls[i].is_blinking) {
            if (current_time - blink_controls[i].last_toggle_time >= blink_controls[i].interval_ms) {
                blink_controls[i].last_toggle_time = current_time;
                blink_controls[i].current_state = !blink_controls[i].current_state;
                
                // 根据当前状态设置颜色
                if (blink_controls[i].current_state) {
                    bsp_LED_Host_SetColor((Host_LED_Type_t)i, 
                        blink_controls[i].blink_red,
                        blink_controls[i].blink_green,
                        blink_controls[i].blink_blue);
                } else {
                    bsp_LED_Host_SetColor((Host_LED_Type_t)i, 0, 0, 0);
                }
            }
        }
    }
}
