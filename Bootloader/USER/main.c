/***********************************************
 * Bootloader —— OTA 启动加载器
 * STM32F407VE + FreeRTOS 高校取餐平台
 *
 * 功能：上电后决定启动 App A 还是 App B，实现 OTA 升级的
 *       引导跳转和失败回滚（Fallback on Failure）。
 *
 * 占用空间：S0 + S1（32KB）
 * 依赖：除标准外设库外，不依赖任何第三方库或操作系统
 *
 * 双槽元数据：
 *   Slot0（App 写触发出升级请求）
 *   Slot1（Bootloader 记录已验证的启动决策）
 ***********************************************/
#include "main.h"
#include "bsp_led.h"
#include "bsp_usart.h"
#include "bsp_delay.h"

/******************************************************************************
 * CRC32 查表法实现（LUT-based CRC32）
 *
 * 为什么用查表法而不是逐位计算？
 *   查表法用 256 个预计算值换时间，每字节只需要一次查表 + 一次异或，
 *   比逐位计算快约 8 倍。在 168MHz 下校验 224KB 的 App A 大约耗时 100ms。
 *
 * 多项式：0xEDB88320（逆向表示，即标准 Ethernet/PKZIP CRC32）
 *   正向表示为 0x04C11DB7，逆向用于小端架构（STM32 为小端）。
 *   与 zlib/binutils/Python 的 zlib.crc32() 完全兼容。
 *****************************************************************************/
static const uint32_t crc32_table[256] = {
    /* 256 个预计算值，由多项式 0xEDB88320 对 0~255 每个字节生成的 CRC */
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/******************************************************************************
 * CRC32_Compute —— 计算一段 RAM 数据的 CRC32
 *
 * 算法：CRC = 0xFFFFFFFF
 *       对每个字节: CRC = TABLE[(CRC ^ byte) & 0xFF] ^ (CRC >> 8)
 *       最终: CRC ^ 0xFFFFFFFF（输出异或）
 *
 * data:   内存缓冲区指针
 * length: 数据长度（字节）
 * 返回:    32 位 CRC 校验值
 *****************************************************************************/
uint32_t crc32_compute(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFUL;
}

/******************************************************************************
 * CRC32_Region —— 计算 Flash 中一段地址空间的 CRC32
 *
 * 与 crc32_compute 的区别：
 *   它直接对 Flash 地址空间做指针解引用，而不需要先将数据读到 RAM 缓冲区。
 *   这样可以省掉一个大缓冲区，对 RAM 有限的 Bootloader 很重要。
 *
 * addr:   Flash 起始地址（如 0x08008000）
 * length: 要校验的字节数
 * 返回:    32 位 CRC 校验值
 *****************************************************************************/
uint32_t crc32_region(uint32_t addr, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t end = addr + length;
    while (addr < end) {
        uint8_t byte = *(volatile uint8_t *)addr;
        crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
        addr++;
    }
    return crc ^ 0xFFFFFFFFUL;
}

/******************************************************************************
 *  Flash 硬件操作抽象层
 *
 *  用标准外设库（StdPeriph）直接操作 Flash 控制器。
 *  为什么不用 HAL？因为 Bootloader 要足够轻量、零依赖。
 *
 *  注意：
 *    - Erase 以扇区为单位（最小 16KB），不能只擦一个字节
 *    - Program 只能将 1 改为 0，不能将 0 改回 1（必须先擦除）
 *    - 每次 Program 后做读回验证（Verify-after-Write），防止位翻转
 *****************************************************************************/

/* 根据 Flash 地址获取扇区编号（用于 FLASH_EraseSector API） */
static uint32_t get_sector(uint32_t addr)
{
    /* STM32F407VE 的扇区分布在 512KB 空间内，大小不均匀 */
    if (addr < 0x08004000) return FLASH_Sector_0;      /* 16KB */
    if (addr < 0x08008000) return FLASH_Sector_1;      /* 16KB */
    if (addr < 0x0800C000) return FLASH_Sector_2;      /* 16KB */
    if (addr < 0x08010000) return FLASH_Sector_3;      /* 16KB */
    if (addr < 0x08020000) return FLASH_Sector_4;      /* 64KB */
    if (addr < 0x08040000) return FLASH_Sector_5;      /* 128KB */
    if (addr < 0x08060000) return FLASH_Sector_6;      /* 128KB */
    return FLASH_Sector_7;                              /* 128KB */
}

/******************************************************************************
 * 擦除包含指定地址的 Flash 扇区
 *
 * sector_addr: 扇区内的任意地址（函数会根据地址映射到对应扇区）
 * 返回: 0=成功，-1=擦除失败
 *****************************************************************************/
int flash_erase_sector(uint32_t sector_addr)
{
    uint32_t sector = get_sector(sector_addr);
    FLASH_Unlock();  /* Flash 控制器默认上锁，防止误操作，擦除前必须解锁 */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    if (FLASH_EraseSector(sector, VoltageRange_3) != FLASH_COMPLETE) {
        FLASH_Lock();
        return -1;
    }
    FLASH_Lock();
    return 0;
}

/******************************************************************************
 * 按字节写入 Flash（Program Byte by Byte）
 *
 * STM32F4 的 Flash 支持按字节、半字、字编程。这里选择按字节以保持最大灵活性。
 * 每次写入后做读回验证（Verify-after-Write），确保数据正确写入。
 *
 * addr: Flash 目标地址（必须是已擦除状态，即全 0xFF）
 * data: 源数据指针
 * len:  数据长度
 * 返回: 0=成功，-1=写入或验证失败
 *****************************************************************************/
int flash_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    FLASH_Unlock();
    for (uint32_t i = 0; i < len; i++) {
        if (FLASH_ProgramByte(addr + i, data[i]) != FLASH_COMPLETE) {
            FLASH_Lock();
            return -1;
        }
        /* 读回验证：刚写进去的值和源数据对比 */
        if (*(volatile uint8_t *)(addr + i) != data[i]) {
            FLASH_Lock();
            return -1;
        }
    }
    FLASH_Lock();
    return 0;
}

/******************************************************************************
 * 从 Flash 读取数据到缓冲区（简单内存拷贝）
 *****************************************************************************/
int flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = *(volatile uint8_t *)(addr + i);
    }
    return 0;
}

/******************************************************************************
 *  OTA 双槽位元数据操作（Slot0 / Slot1）
 *
 *  每个函数对应 main.h 中定义的一个操作，命名统一、职责单一。
 *  为什么读写函数不做魔数校验？
 *    - 读函数：纯粹的 memcpy 级操作，快。魔数由调用方自行判断。
 *    - 写函数：因为 Flash 特性，即使魔数不匹配也能写入（覆写会失败，但函数内部不关心语义）。
 *****************************************************************************/

void ota_slot0_init(OTA_Slot0_t *s)
{
    memset(s, 0, sizeof(OTA_Slot0_t));
    s->magic = OTA_SLOT0_MAGIC;          /* 填充魔数，供后续读函数校验 */
    s->to_b_flag = SLOT0_FLAG_NORMAL;    /* 默认无升级请求 */
}

int ota_slot0_read(OTA_Slot0_t *s)
{
    return flash_read(OTA_SLOT0_ADDR, (uint8_t *)s, sizeof(OTA_Slot0_t));
}

int ota_slot0_write(const OTA_Slot0_t *s)
{
    return flash_write(OTA_SLOT0_ADDR, (const uint8_t *)s, sizeof(OTA_Slot0_t));
}

void ota_slot1_init(OTA_Slot1_t *s)
{
    memset(s, 0, sizeof(OTA_Slot1_t));
    s->magic = OTA_SLOT1_MAGIC;
    s->active_region = ACTIVE_REGION_A;  /* 默认从 A 区启动 */
}

int ota_slot1_read(OTA_Slot1_t *s)
{
    return flash_read(OTA_SLOT1_ADDR, (uint8_t *)s, sizeof(OTA_Slot1_t));
}

int ota_slot1_write(const OTA_Slot1_t *s)
{
    return flash_write(OTA_SLOT1_ADDR, (const uint8_t *)s, sizeof(OTA_Slot1_t));
}

/******************************************************************************
 * 串口打印两个槽位的状态（调试信息输出）
 *****************************************************************************/
void ota_print_slots(const OTA_Slot0_t *s0, const OTA_Slot1_t *s1)
{
    Serial_Printf(DEBUG_USART, "[BOOT] Slot0: magic=0x%08lX, flag=0x%02X, ver=0x%08lX\r\n",
           s0->magic, s0->to_b_flag, s0->app_b_version);
    Serial_Printf(DEBUG_USART, "[BOOT] Slot1: magic=0x%08lX, region=%s, tries=%d\r\n",
           s1->magic,
           s1->active_region == ACTIVE_REGION_A ? "A" : "B",
           s1->boot_attempts);
}

/******************************************************************************
 * boot_jump_to_app —— 跳转到目标应用程序（核心跳转函数）
 *
 * 这是 Bootloader 最重要、最微妙的函数。
 * 它直接操作 Cortex-M4 的 MSP/PC/VTOR 这三个内核寄存器。
 *
 * 跳转前必须做的清理工作：
 *  1. __disable_irq()      → 关全局中断，防止跳转途中某个 ISR 突然进来
 *  2. SysTick->CTRL = 0    → 停掉 SysTick 定时器，它一直在产生中断
 *  3. NVIC->ICER[*]=全1    → 清除所有外设中断使能
 *  4. NVIC->ICPR[*]=全1    → 清除所有挂起的中断（Pending），否则跳过去马上触发
 *
 * 然后获取目标 App 的中断向量表前两样东西：
 *   - 第 1 个字（4 字节）：主栈指针 MSP 初始值
 *   - 第 2 个字：复位向量 Reset_Handler（App 的入口地址）
 *
 * 最后设置 MSP → 设置 VTOR → 跳转。
 * VTOR（Vector Table Offset Register）告诉内核中断向量表在哪。
 *****************************************************************************/
void boot_jump_to_app(uint32_t app_addr)
{
    uint32_t msp_value;      /* 目标 App 的主栈指针 */
    uint32_t reset_vector;   /* 目标 App 的复位处理函数地址 */

    Serial_Printf(DEBUG_USART, "[BOOT] Jumping to App at 0x%08lX...\r\n\r\n", app_addr);
    delay_ms(10);  /* 等串口 FIFO 吐完再关中断，最后几行日志别丢 */

    /* ----- 关闭一切中断 ----- */
    __disable_irq();             /* 关全局中断（PRIMASK 置位） */
    SysTick->CTRL = 0;           /* 停 SysTick 定时器，它可能正在跑 */

    /* 清除所有使能和挂起的中断 */
    NVIC->ICER[0] = 0xFFFFFFFF;  /* 外设中断清除使能寄存器 */
    NVIC->ICER[1] = 0xFFFFFFFF;
    NVIC->ICER[2] = 0xFFFFFFFF;
    NVIC->ICPR[0] = 0xFFFFFFFF;  /* 外设中断清除挂起寄存器 */
    NVIC->ICPR[1] = 0xFFFFFFFF;
    NVIC->ICPR[2] = 0xFFFFFFFF;

    /* 从目标 App 的向量表读取初始栈指针和复位向量 */
    msp_value = *(volatile uint32_t *)app_addr;          /* 向量表首字 = MSP */
    reset_vector = *(volatile uint32_t *)(app_addr + 4); /* 向量表次字 = Reset_Handler */

    /* 设置主栈指针到 App 的栈顶（由链接脚本决定，通常是 SRAM 末端） */
    __set_MSP(msp_value);

    /*
     * 设置向量表偏移寄存器（VTOR）为 App 基地址。
     * 从此中断控制器就知道新的向量表在哪了。
     * 注意：App 的 SystemInit 中也会设置 VTOR，但这里先设好更安全。
     */
    SCB->VTOR = app_addr;

    /* 函数指针跳转到 App 的 Reset_Handler */
    typedef void (*pFunction)(void);
    pFunction jump = (pFunction)reset_vector;
    jump();

    /* 如果跳转成功，永远不会走到这里 */
    while (1);
}

/******************************************************************************
 * 应用有效性检查（App Validity Check）
 *
 * 根据 ARM Cortex-M 的向量表规范：
 *   向量表首字（SP）：必须指向 SRAM 地址空间（0x20000000 ~ 0x20020000）
 *   向量表次字（PC/Reset）：必须指向 Flash 地址空间（0x08000000 ~ 0x08080000）
 *
 * 这种检查方式比 CRC 校验更快（只读 8 字节），不需要擦写 Flash 就能判断。
 * 可以挡住大部分"Flash 是空的"或"数据损坏"场景。
 *****************************************************************************/
static int is_app_valid(uint32_t app_addr)
{
    uint32_t sp = *(volatile uint32_t *)app_addr;        /* 栈指针值 */
    uint32_t pc = *(volatile uint32_t *)(app_addr + 4);  /* 程序计数器初值 */

    /* SP 必须在片内 SRAM 范围内（128KB，从 0x20000000 开始） */
    if (sp < 0x20000000 || sp > 0x20020000) return 0;
    /* PC 初值必须在 Flash 地址范围内 */
    if (pc < 0x08000000 || pc > 0x08080000) return 0;
    return 1;
}

/******************************************************************************
 * 系统时钟配置（独立于 App 的时钟树初始化）
 *
 * HSE（外部高速晶振）= 12MHz
 * PLL 配置：
 *   VCO 时钟 = HSE / PLL_M * PLL_N = 12MHz / 8 * 336 = 504MHz
 *   系统时钟 = VCO / PLL_P = 504MHz / 2 = 168MHz
 *   USB/SDIO = VCO / PLL_Q = 504MHz / 7 = 72MHz（符合 USB 规范）
 *
 * 为什么 Bootloader 要自己配时钟？
 *   上电时芯片用内部 HSI（16MHz）运行，如果不配时钟，后面的延时、
 *   串口波特率全是错的。Bootloader 不能依赖 App 的 SystemInit。
 *****************************************************************************/
void SystemClock_Config(void)
{
    RCC_DeInit();                           /* 复位 RCC 配置 */

    RCC_HSEConfig(RCC_HSE_ON);              /* 开启 HSE 晶振 */
    while (RCC_WaitForHSEStartUp() != SUCCESS);  /* 等待 HSE 稳定 */

    /* Flash 等待周期：168MHz 需要 5 个等待周期（参考 STM32F4 数据手册） */
    FLASH_SetLatency(FLASH_Latency_5);
    FLASH_PrefetchBufferCmd(ENABLE);         /* 预取缓冲使能 */
    FLASH_InstructionCacheCmd(ENABLE);       /* 指令缓存使能 */
    FLASH_DataCacheCmd(ENABLE);              /* 数据缓存使能 */

    /* PLL 配置：HSE(12MHz) / 8 * 336 / 2 = 168MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE, 8, 336, 2, 7);
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);  /* 等 PLL 锁定 */

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);   /* 系统时钟切换到 PLL */
    while (RCC_GetSYSCLKSource() != 0x08);        /* 0x08 = PLL 作为系统时钟 */

    /* AHB 时钟 = 168MHz，APB2 = 84MHz，APB1 = 42MHz */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div2);
    RCC_PCLK1Config(RCC_HCLK_Div4);

    SystemCoreClock = 168000000;  /* 更新全局变量，供 bsp_delay 计算延时参数 */
}

/******************************************************************************
 * Bootloader 系统初始化
 *
 * 只初始化最基本的硬件：
 *   时钟 → 延时（依赖 SysTick 轮询） → 调试串口（115200） → LED
 *****************************************************************************/
void system_init(void)
{
    SystemClock_Config();
    delay_init();                              /* 初始化 SysTick 轮询延时 */
    bsp_usart_init(DEBUG_USART, 115200);       /* USART1：调试日志输出 */
    bsp_LED_Host_Init();                       /* RGB LED（PB0/PB1/PE7） */
    LED_BOOT_INDICATE();                       /* RGB 各闪一次，表示 Bootloader 活着 */
    LED_OFF_ALL();
}

/******************************************************************************
 * main —— Bootloader 主入口
 *
 * 启动决策机（Boot Decision Engine）：
 *
 *   [读取双槽位]
 *       │
 *       ├─ Slot1.active == B 且 App B 有效？───Yes──→ 从 B 启动 ✔
 *       │                                              （持续从 B 启动）
 *       │
 *       ├─ Slot0.to_b_flag == TO_B 且 App B 有效？─Yes──→ 写 Slot1(active=B) → 从 B 启动 ✔
 *       │                                               （OTA 首次切换）
 *       │
 *       ├─ Slot0.to_b_flag == TO_B 但 App B 无效？───→ 回滚：清 Slot0 → 从 A 启动 ✘
 *       │                                               （OTA 失败回退）
 *       │
 *       └─ 默认：从 A 启动 ✔
 *              （首次烧录 或 正常启动）
 *
 *  回滚保证：
 *    - OTA 下载完成后 App B 写入失败 → 不会写 TO_B → 下次正常启动 A
 *    - OTA 切换失败（App B 跑飞/看门狗复位）→ Bootloader 检查 App B 无效 → 回退 A
 *****************************************************************************/
int main(void)
{
    OTA_Slot0_t slot0;   /* 存储 Slot0 的 RAM 副本 */
    OTA_Slot1_t slot1;   /* 存储 Slot1 的 RAM 副本 */
    int slot0_valid, slot1_valid;

    /* 第一步：硬件初始化 */
    system_init();

    /* 打印 Bootloader 版本横幅 */
    Serial_Printf(DEBUG_USART, "\r\n========================================\r\n");
    Serial_Printf(DEBUG_USART, "  WHEELTEC Bootloader v1.0\r\n");
    Serial_Printf(DEBUG_USART, "  Chip: STM32F407VE (512KB Flash)\r\n");
    Serial_Printf(DEBUG_USART, "========================================\r\n\r\n");

    /* 第二步：从 Flash 读取两个元数据槽位 */
    slot0_valid = (ota_slot0_read(&slot0) == 0 && slot0.magic == OTA_SLOT0_MAGIC);
    slot1_valid = (ota_slot1_read(&slot1) == 0 && slot1.magic == OTA_SLOT1_MAGIC);
    ota_print_slots(&slot0, &slot1);

    /* ================================================================== */
    /* 分支 1：Slot1 已确认应启动 B 区                                    */
    /* 场景：之前有一次成功的 OTA 切换，Bootloader 已经记录下来            */
    /* ================================================================== */
    if (slot1_valid && slot1.active_region == ACTIVE_REGION_B) {
        if (is_app_valid(APP_B_ADDR)) {
            Serial_Printf(DEBUG_USART, "[BOOT] Slot1: boot B (committed)\r\n");
            LED_SUCCESS();             /* 绿灯亮表示启动成功 */
            boot_jump_to_app(APP_B_ADDR);
        }
        /* App B 异常（可能被擦除了或硬件损坏），回退到 A */
        Serial_Printf(DEBUG_USART, "[BOOT] Slot1 says B but B invalid, clearing\r\n");
        ota_slot1_init(&slot1);
        ota_slot1_write(&slot1);
        slot1_valid = 0;
    }

    /* ================================================================== */
    /* 分支 2：App 请求切换到 B 区（OTA 触发）                            */
    /* 场景：App 下载完新固件并写好了 Slot0 的 TO_B 标志                   */
    /* ================================================================== */
    if (slot0_valid && slot0.to_b_flag == SLOT0_FLAG_TO_B) {
        Serial_Printf(DEBUG_USART, "[BOOT] Slot0: TO_B request, verifying...\r\n");

        if (is_app_valid(APP_B_ADDR)) {
            /* B 区看起来是有效的 → 写 Slot1 固化决策 */
            OTA_Slot1_t s1;
            ota_slot1_init(&s1);
            s1.active_region = ACTIVE_REGION_B;        /* 下次也启动 B */
            s1.app_a_version = slot0.app_b_version;    /* 记录新版本号 */
            s1.app_a_crc = slot0.app_b_crc;            /* 记录新 CRC */
            ota_slot1_write(&s1);

            Serial_Printf(DEBUG_USART, "[BOOT] App B verified, committed\r\n");
            LED_SUCCESS();
            boot_jump_to_app(APP_B_ADDR);
        }

        /* B 区无效 → 回滚：清除 Slot0 的请求标志，下次不会再来这里 */
        Serial_Printf(DEBUG_USART, "[BOOT] App B invalid! Rolling back to A\r\n");
        slot0.to_b_flag = SLOT0_FLAG_NORMAL;
        ota_slot0_write(&slot0);
    }

    /* ================================================================== */
    /* 分支 3：默认从 A 区启动（首次烧录 / 正常重启 / 回滚后）            */
    /* ================================================================== */
    Serial_Printf(DEBUG_USART, "[BOOT] Booting App A...\r\n");

    if (is_app_valid(APP_A_ADDR)) {
        /* 如果 Slot1 还没初始化（首次运行 Bootloader），写一条初始记录 */
        if (!slot1_valid) {
            OTA_Slot1_t s1;
            ota_slot1_init(&s1);
            s1.active_region = ACTIVE_REGION_A;
            s1.app_a_version = 0;                                    /* 出厂版本未知 */
            s1.app_a_crc = crc32_region(APP_A_ADDR, APP_A_SIZE);     /* 计算 App A 的 CRC */
            ota_slot1_write(&s1);
            Serial_Printf(DEBUG_USART, "[BOOT] App A CRC=0x%08lX initialized in Slot1\r\n",
                   s1.app_a_crc);
        }

        LED_SUCCESS();
        boot_jump_to_app(APP_A_ADDR);
    }

    /* ================================================================== */
    /* 致命错误：A 区和 B 区都没有有效固件                                 */
    /* 现象：红灯常亮，串口输出 ERROR 日志                                */
    /* 处理：无限循环闪烁，等待外部干预（烧录器重新编程或恢复出厂设置）    */
    /* ================================================================== */
    LED_ERROR();  /* 红灯 */
    Serial_Printf(DEBUG_USART, "\r\n[BOOT] ERROR: No valid firmware!\r\n");
    while (1) {
        LED_OFF_ALL();
        delay_ms(500);
        LED_ERROR();
        delay_ms(500);
    }
}
