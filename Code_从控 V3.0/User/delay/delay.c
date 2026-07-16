#include "stm32f10x.h"
#include "delay/delay.h"

//延时系数
unsigned char UsCount = 0;
unsigned short MsCount = 0;

static u32 sysTickCnt=0;

/**
 * @brief 获取系统tick计数
 * @return 返回系统启动后的tick计数
 * @note 如果FreeRTOS调度器已启动，返回xTaskGetTickCount()
 *       如果FreeRTOS调度器未启动，返回内部维护的sysTickCnt
 */
u32 getSysTickCnt(void)
{
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)	/*系统已经运行*/
		return xTaskGetTickCount();
	else
		return sysTickCnt;
}	

/**
 * @brief 延时初始化函数
 * @note 配置SysTick时钟为HCLK/8 = 9MHz (F103默认)
 *       设置微秒和毫秒延时系数
 *       配置FreeRTOS系统节拍中断
 */
void Delay_Init(void)
{
    SysTick->CTRL &= ~(1 << 2);  // 选择时钟源为HCLK/8 = 9MHz
    UsCount = 9;                 // 9MHz时钟，1us需要9个计数
    MsCount = UsCount * 1000;    // 1ms需要9000个计数
    
    // 添加FreeRTOS配置
    uint32_t reload = 9 * 1000000 / configTICK_RATE_HZ; // 计算FreeRTOS节拍重载值
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;          // 开启SysTick中断
    SysTick->LOAD = reload;                             // 设置重载值
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;           // 启动SysTick计数器
}

/**
 * @brief 同步毫秒延时函数
 * @param ms 要延时的毫秒数
 * @note 此函数在FreeRTOS调度器运行前后都能工作
 *       - 调度器运行时：使用vTaskDelay，不阻塞系统
 *       - 调度器未运行：使用DelayMs阻塞延时
 *       确保与主机延时同步
 */
void Delay_Ms_Sync(uint32_t ms)
{
    if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        // FreeRTOS调度器已运行，使用非阻塞任务延时
        TickType_t ticks = pdMS_TO_TICKS(ms);  // 将毫秒转换为tick数
        if (ticks == 0 && ms > 0) {
            ticks = 1;  // 确保至少延时1个tick
        }
        vTaskDelay(ticks);  // FreeRTOS非阻塞延时
    } else {
        // FreeRTOS调度器未运行，使用原有的阻塞延时
        DelayMs(ms);
    }
}



/*********************************阻塞式*******************************/

//函数名称:Delay_Init
//函数参数:void
//函数返回:void
//函数功能:延迟函数初始化(阻塞)
//void Delay_Init(void)
//{

//	SysTick->CTRL &= ~(1 << 2);		//选择时钟为HCLK(72MHz)/8		103--9MHz
//	
//	UsCount = 9;					//微秒级延时系数
//	
//	MsCount = UsCount * 1000;		//毫秒级延时系数

//}

//函数名称:delay_us
//函数参数:unsigned short us
//函数返回:void
//函数功能:延迟微秒级
void Delay_us(unsigned short us)
{

	unsigned int ctrlResult = 0;
	
	us &= 0x00FFFFFF;											//取低24位
	
	SysTick->LOAD = us * UsCount;								//装载数据
	SysTick->VAL = 0;
	SysTick->CTRL = 1;											//使能倒计数器
	
	do
	{
		ctrlResult = SysTick->CTRL;
	}
	while((ctrlResult & 0x01) && !(ctrlResult & (1 << 16)));	//保证在运行、检查是否倒计数到0
	
	SysTick->CTRL = 0;											//关闭倒计数器
	SysTick->VAL = 0;

}



//函数名称:delay_ms
//函数参数:unsigned short ms
//函数返回:void
//函数功能:延迟毫秒级
void Delay_ms(unsigned short ms)
{

	unsigned int ctrlResult = 0;
	
	if(ms == 0)
		return;
	
	ms &= 0x00FFFFFF;											//取低24位
	
	SysTick->LOAD = ms * MsCount;								//装载数据
	SysTick->VAL = 0;
	SysTick->CTRL = 1;											//使能倒计数器
	
	do
	{
		ctrlResult = SysTick->CTRL;
	}
	while((ctrlResult & 0x01) && !(ctrlResult & (1 << 16)));	//保证在运行、检查是否倒计数到0
	
	SysTick->CTRL = 0;											//关闭倒计数器
	SysTick->VAL = 0;

}


//函数名称:DelayMs
//函数参数:unsigned short ms
//函数返回:void
//函数功能:另一个延迟毫秒级
void DelayMs(unsigned short ms)
{

	unsigned char repeat = 0;
	unsigned short remain = 0;
	
	repeat = ms / 500;
	remain = ms % 500;
	
	while(repeat)
	{
		Delay_ms(500);
		repeat--;
	}
	
	if(remain)
		Delay_ms(remain);
}



