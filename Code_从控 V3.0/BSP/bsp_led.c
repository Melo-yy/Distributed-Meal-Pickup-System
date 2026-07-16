#include "bsp_led.h"

static slave_led_blink_ctrl_t slave_blink_control = {0};

/**************************************************************************
Function: LED interface initialization
Input   : none
Output  : none
函数功能：LED接口初始化
入口参数：无 
返回  值：无
**************************************************************************/
void bsp_LED_Init(void)
{    
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 使能从控LED时钟
    RCC_APB2PeriphClockCmd(SLAVE_LED_GPIO_CLK, ENABLE);

    // 2. 配置从控RGB LED
    GPIO_InitStructure.GPIO_Pin = SLAVE_LED_RED_PIN | SLAVE_LED_GREEN_PIN | SLAVE_LED_BLUE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SLAVE_LED_GPIO_PORT, &GPIO_InitStructure);

    // 3. 初始化LED为关闭状态
    bsp_LED_Slave_SetColor(SLAVE_LED_POWER, 0, 0, 0);
    
    // 4. 初始化闪烁控制结构
    memset(&slave_blink_control, 0, sizeof(slave_blink_control));
    
}

/**************************************************************************
Function: 设置从控LED颜色
Input   : red, green, blue - LED状态 (0:关, 1:开)
Output  : none
**************************************************************************/
void bsp_LED_Slave_SetColor(Slave_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue) 
{
    // 停止闪烁
    slave_blink_control.is_blinking = 0;
    
    if (led_type == SLAVE_LED_POWER) {
        red   ? SLAVE_LED_RED_ON()   : SLAVE_LED_RED_OFF();
        green ? SLAVE_LED_GREEN_ON() : SLAVE_LED_GREEN_OFF();
        blue  ? SLAVE_LED_BLUE_ON()  : SLAVE_LED_BLUE_OFF();
    }
}

/**************************************************************************
Function: 设置从控LED固定颜色
Input   : red, green, blue - 颜色
Output  : none
**************************************************************************/
void bsp_LED_Slave_SetSolidColor(Slave_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue)
{
    bsp_LED_Slave_SetColor(led_type, red, green, blue);
}

/**************************************************************************
Function: 设置从控LED闪烁颜色
Input   : red, green, blue - 颜色, interval_ms - 闪烁间隔
Output  : none
**************************************************************************/
void bsp_LED_Slave_SetBlinkColor(Slave_LED_Type_t led_type, uint8_t red, uint8_t green, uint8_t blue, uint32_t interval_ms)
{
    if (led_type == SLAVE_LED_POWER) {
        slave_blink_control.is_blinking = 1;
        slave_blink_control.interval_ms = interval_ms;
        slave_blink_control.last_toggle_time = getSysTickCnt();  // 使用您的系统时钟函数
        slave_blink_control.current_state = 1;
        slave_blink_control.blink_red = red;
        slave_blink_control.blink_green = green;
        slave_blink_control.blink_blue = blue;
        
        // 初始状态为亮
        bsp_LED_Slave_SetColor(led_type, red, green, blue);
    }
}

/**************************************************************************
Function: 更新从控LED闪烁状态
Input   : none
Output  : none
**************************************************************************/
void bsp_LED_Slave_UpdateBlink(void)
{
    if (!slave_blink_control.is_blinking) {
        return;
    }
    
    uint32_t current_time = getSysTickCnt();  // 使用您的系统时钟函数
    if (current_time - slave_blink_control.last_toggle_time >= slave_blink_control.interval_ms) {
        slave_blink_control.last_toggle_time = current_time;
        slave_blink_control.current_state = !slave_blink_control.current_state;
        
        // 根据当前状态设置颜色
        if (slave_blink_control.current_state) {
            bsp_LED_Slave_SetColor(SLAVE_LED_POWER, 
                slave_blink_control.blink_red,
                slave_blink_control.blink_green,
                slave_blink_control.blink_blue);
        } else {
            bsp_LED_Slave_SetColor(SLAVE_LED_POWER, 0, 0, 0);
        }
    }
}


