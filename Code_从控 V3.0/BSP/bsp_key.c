#include "bsp_key.h"

// 按键状态机状态
typedef enum {
    KEY_STATE_IDLE = 0,       // 空闲状态
    KEY_STATE_DEBOUNCE,       // 消抖状态
    KEY_STATE_PRESSED,        // 已按下状态
    KEY_STATE_LONG_PRESSED,   // 长按已触发状态 ← 新增状态
    KEY_STATE_RELEASE,        // 释放状态
    KEY_STATE_WAIT_DOUBLE     // 等待第二次按下（判断双击）
} Key_State_t;

// 静态变量（保持状态）
static Key_State_t key_state = KEY_STATE_IDLE;
static uint32_t key_press_time = 0;
static uint32_t key_release_time = 0;
static uint8_t click_count = 0;

/**************************************************************************
Function: Key initialization
Input   : none
Output  : none
函数功能：按键初始化
入口参数：无
返回  值：无 
**************************************************************************/
void bsp_KEY_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(KEY_GPIO_CLK, ENABLE);//使能GPIOB时钟
	GPIO_InitStructure.GPIO_Pin = KEY_GPIO_PIN; //KEY对应引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStructure);//初始化GPIOE0	
} 


/**************************************************************************
Function: Read key current state
Input   : none
Output  : Key state (KEY_ON or KEY_OFF)
函数功能：读取按键当前状态
入口参数：无
返回  值：按键状态 (KEY_ON 或 KEY_OFF)
**************************************************************************/
uint8_t bsp_Key_Read(void)
{
    return GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_GPIO_PIN);
}


/**************************************************************************
Function: Non-blocking key scan (state machine)
Input   : none
Output  : Key event type
函数功能：非阻塞按键扫描（状态机方式）
入口参数：无
返回  值：按键事件类型
**************************************************************************/
Key_Event_t bsp_Key_Scan(void)
{
    uint32_t current_time = getSysTickCnt();        // 获取当前系统时间(ms)
    uint8_t key_current_state = bsp_Key_Read();     // 读取按键当前物理状态
    Key_Event_t event = KEY_EVENT_NONE;             // 默认无事件
    
    // 状态机处理
    switch (key_state) {
        // 状态1：空闲状态 - 等待按键按下
        case KEY_STATE_IDLE:
            if (key_current_state == KEY_ON) {      // 检测到按键按下
                key_state = KEY_STATE_DEBOUNCE;     // 转移到消抖状态
                key_press_time = current_time;      // 记录按下时间戳
            }
            break;
            
        // 状态2：消抖状态 - 等待20ms消除机械抖动
        case KEY_STATE_DEBOUNCE:
            if (current_time - key_press_time > 20) {  // 消抖时间到
                if (key_current_state == KEY_ON) {     // 按键仍然按下，确认有效按下
                    key_state = KEY_STATE_PRESSED;     // 转移到按下状态
                    click_count++;                     // 单击计数增加
                } else {                               // 按键已释放，认为是抖动
                    key_state = KEY_STATE_IDLE;        // 返回空闲状态
                }
            }
            break;
            
        // 状态3：按下状态 - 按键已确认按下，等待释放或长按
        case KEY_STATE_PRESSED:
            if (key_current_state == KEY_OFF) {        // 检测到按键释放
                key_state = KEY_STATE_RELEASE;         // 转移到释放状态
                key_release_time = current_time;       // 记录释放时间戳
            } 
            // 检测长按（3000ms = 3秒）
            else if (current_time - key_press_time > 3000) {
                key_state = KEY_STATE_LONG_PRESSED;    // 转移到长按状态
                event = KEY_EVENT_LONG_PRESS;          // 返回长按事件
            }
            break;
            
        // 状态4：长按状态 - 长按事件已触发，等待按键释放
        case KEY_STATE_LONG_PRESSED:
            if (key_current_state == KEY_OFF) {        // 检测到按键释放
                key_state = KEY_STATE_IDLE;            // 返回空闲状态
                click_count = 0;                       // 重置单击计数
            }
            // 如果需要长按保持连续触发，可以在这里添加逻辑
            // 例如：else if (current_time - key_press_time > 5000) {
            //     event = KEY_EVENT_LONG_HOLD;       // 每5秒触发一次长按保持
            // }
            break;
            
        // 状态5：释放状态 - 按键已释放，进行释放消抖
        case KEY_STATE_RELEASE:
            if (current_time - key_release_time > 20) {  // 释放消抖时间到
                if (click_count == 1) {                // 如果是第一次按下
                    key_state = KEY_STATE_WAIT_DOUBLE; // 转移到等待双击状态
                } else {                               // 其他情况
                    key_state = KEY_STATE_IDLE;        // 返回空闲状态
                    click_count = 0;                   // 重置单击计数
                }
            }
            break;
            
        // 状态6：等待双击状态 - 等待500ms判断是否第二次按下
        case KEY_STATE_WAIT_DOUBLE:
            if (current_time - key_release_time > 500) {  // 双击超时（500ms）
                key_state = KEY_STATE_IDLE;            // 返回空闲状态
                if (click_count == 1) {                // 如果只有一次按下
                    click_count = 0;                   // 重置计数
                    event = KEY_EVENT_CLICK;           // 返回单击事件
                }
            } else if (key_current_state == KEY_ON) {  // 在超时时间内再次按下
                key_state = KEY_STATE_DEBOUNCE;        // 返回消抖状态
                key_press_time = current_time;         // 更新按下时间
            }
            break;
    }
    
    // 双击检测：在状态机外部判断，因为可能在任何状态触发
    if (click_count == 2) {              // 检测到两次单击
        click_count = 0;                 // 重置单击计数
        key_state = KEY_STATE_IDLE;      // 返回空闲状态
        event = KEY_EVENT_DOUBLE;        // 返回双击事件
    }
    
    return event;  // 返回检测到的事件（如果没有事件则返回KEY_EVENT_NONE）
}
