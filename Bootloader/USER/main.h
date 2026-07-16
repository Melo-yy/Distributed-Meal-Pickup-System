#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 *                          Flash 分区布局（Memory Map）
 *
 * STM32F407VET6 片内 Flash 共 512KB，地址空间 0x08000000 ~ 0x0807FFFF。
 * Flash 的擦除单位是扇区（Sector），不同扇区大小不同：
 *
 *   0x08000000 ┌─────────────────────────┐
 *              │  Bootloader (32KB)       │  S0(16KB) + S1(16KB)
 *   0x08008000 ├─────────────────────────┤
 *              │  App A (224KB)           │  S2(16KB)+S3(16KB)+S4(64KB)+S5(128KB)
 *   0x08040000 ├─────────────────────────┤
 *              │  App B (124KB)           │  S6(128KB)，OTA 固件下载目标
 *              │  ┌───────────────────┐   │
 *              │  │ Slot0 @0x0805F000 │   │  64B：App 写 → 触发升级请求
 *              │  │ Slot1 @0x0805F100 │   │  64B：Bootloader 写 → 记录决策
 *              │  └───────────────────┘   │
 *   0x08060000 ├─────────────────────────┤
 *              │  fdb_kvdb1 (128KB)       │  S7(128KB)，FlashDB 键值数据库
 *   0x0807FFFF └─────────────────────────┘
 *****************************************************************************/

#define FLASH_BASE_ADDR              0x08000000UL    /* Flash 起始地址（物理基址） */

/* Bootloader：占用 S0 + S1，共 32KB */
#define BOOTLOADER_SIZE              (32UL * 1024)

/* App A（当前运行的应用程序）：占用 S2 ~ S5，共 224KB */
#define APP_A_ADDR                   (FLASH_BASE_ADDR + BOOTLOADER_SIZE)
#define APP_A_SIZE                   (224UL * 1024)

/* App B（OTA 升级目标区域）：占用 S6，共 128KB */
#define APP_B_ADDR                   0x08040000UL
#define APP_B_SIZE                   (128UL * 1024)

/******************************************************************************
 *  OTA 双槽位元数据（Dual-Slot Metadata）设计说明
 *
 *  Flash 的物理特性：写入（Program）只能将比特从 1 改为 0，
 *  要想把 0 改回 1 必须整扇区擦除（Erase）。
 *  这意味着：同一个地址在两次擦除之间只能写入一次。
 *
 *  解决方案 → 双槽位机制：
 *    在 App B 扇区末尾划出两个独立的 64 字节槽位（不同物理地址）。
 *    - 每次擦除 S6（下载新固件前），两个槽位同时回到全 1（0xFF）状态
 *    - 每个槽位在每次擦除周期内只写一次，物理地址不同 → 无冲突
 *
 *  职责分离：
 *    Slot0（0x0805F000）：App → Bootloader，下游通知上游
 *    Slot1（0x0805F100）：Bootloader → App，上游记录决策
 */
#define OTA_SLOT0_ADDR               0x0805F000UL    /* Slot0：App 写触发，Bootloader 读 */
#define OTA_SLOT1_ADDR               0x0805F100UL    /* Slot1：Bootloader 写决策，App 读 */
#define OTA_SLOT_SIZE                64              /* 每个槽位 64 字节（预留扩展） */

/******************************************************************************
 *  CRC32 校验函数
 *  使用查表法（LUT-based）计算，多项式为 0xEDB88320（Ethernet/PKZIP 标准）。
 *  查表法比逐位计算快约 8 倍，适合在嵌入式环境中对大块数据做完整性校验。
 */
uint32_t crc32_compute(const uint8_t *data, uint32_t length);  /* 计算 RAM 缓冲区的 CRC32 */
uint32_t crc32_region(uint32_t addr, uint32_t length);          /* 计算 Flash 区的 CRC32（直接读 Flash） */

/******************************************************************************
 *  Flash 硬件操作抽象层
 *  直接操作 STM32F4 的 Flash 控制器寄存器（标准外设库 API），不依赖 OS。
 *  这些函数在 Bootloader 中独立存在，不从 App 侧复用代码 → 减少耦合。
 */
int  flash_erase_sector(uint32_t sector_addr);   /* 擦除包含该地址的整个扇区（按扇区对齐擦除） */
int  flash_write(uint32_t addr, const uint8_t *data, uint32_t len);  /* 按字节编程，写后读回验证 */
int  flash_read(uint32_t addr, uint8_t *buf, uint32_t len);          /* 直接从 Flash 地址读取 */

/******************************************************************************
 *  魔数（Magic Number）与标志定义
 *
 *  魔数的作用：Bootloader 上电后读 Flash 中的魔数，如果匹配说明该槽位
 *  已被正确初始化过；如果不匹配（读出 0xFFFFFFFF）说明是空槽位或被损坏。
 *
 *  OTA_SLOT0_MAGIC = 0x4F54414D  ← ASCII "OTAM" 的二进制拼接
 *  OTA_SLOT1_MAGIC = 0x4F544156  ← ASCII "OTAV"（Validated）
 */

/* ---------- Slot0 标志位 ---------- */
#define OTA_SLOT0_MAGIC              0x4F54414DUL   /* 魔数 "OTAM" */
#define SLOT0_FLAG_NORMAL            0x00            /* 无升级请求，正常启动流程 */
#define SLOT0_FLAG_TO_B              0x5A            /* App 请求 Bootloader 切换到 B 区启动 */

/* ---------- Slot1 标志位 ---------- */
#define OTA_SLOT1_MAGIC              0x4F544156UL   /* 魔数 "OTAV"（Bootloader 已验证） */
#define ACTIVE_REGION_A              0               /* 活动分区为 A */
#define ACTIVE_REGION_B              1               /* 活动分区为 B */

/*
 * #pragma pack(1) → 1 字节对齐
 *
 * 编译器默认按成员类型对齐结构体（如 uint32_t 会 4 字节对齐），
 * 结构体成员之间可能出现填充字节（padding）。
 * 这对写入 Flash 有隐患：Bootloader 编译时的对齐方式可能与 App 不同，
 * 导致解析出错误的数据。强制 1 字节对齐保证内存布局跨编译单元一致。
 */
#pragma pack(1)

/*
 * Slot0 结构体 —— 由 App 写入、Bootloader 读取
 *
 * 用于"触发"OTA 升级：
 *   1. App 下载固件 → CRC 校验通过 → 往 Slot0 写 to_b_flag = TO_B
 *   2. App 软复位触发 Bootloader
 *   3. Bootloader 读到 TO_B → 尝试验证 App B → 决定切还是回滚
 */
typedef struct {
    uint32_t    magic;              /* 魔数，必须等于 OTA_SLOT0_MAGIC，否则视为未初始化 */
    uint8_t     to_b_flag;          /* 触发标志：SLOT0_FLAG_NORMAL / SLOT0_FLAG_TO_B */
    uint8_t     reserved1[3];       /* 保留字节，用于 4 字节对齐（配合 pack 确保布局确定） */
    uint32_t    app_b_version;      /* 新固件版本号，低 8 位=patch，次低 8 位=minor，高 16 位=major */
    uint32_t    app_b_crc;          /* 新固件的 CRC32 校验值，Bootloader 据此做完整性判决 */
    uint8_t     reserved[48];       /* 填充到 64 字节，预留未来扩展（如增加签名、时间戳等） */
} OTA_Slot0_t;

/*
 * Slot1 结构体 —— 由 Bootloader 写入、App 可读取
 *
 * 用于"记录"启动决策：
 *   一旦 Bootloader 验证 App B 通过并成功切换，将结果固化到 Slot1。
 *   后续每次复位，Bootloader 优先检查 Slot1 的 active_region。
 *   即使 Slot0 随后被擦除（下一次 OTA），系统仍稳定从 B 区启动。
 *
 *   boot_attempts 字段预留用于"启动失败检测"：
 *   如果应用在启动后无法正常运行（看门狗超时复位），
 *   boot_attempts 递增，达到阈值后可触发自动回滚。
 */
typedef struct {
    uint32_t    magic;              /* 魔数，必须等于 OTA_SLOT1_MAGIC */
    uint8_t     active_region;      /* Bootloader 确认的有效分区：ACTIVE_REGION_A 或 B */
    uint8_t     boot_attempts;      /* 启动尝试次数，用于检测反复复位 → 触发回滚 */
    uint8_t     reserved1[2];       /* 保留字节 */
    uint32_t    app_a_version;      /* 当前运行的固件版本（Bootloader 提交时记录） */
    uint32_t    app_a_crc;          /* 当前运行的固件的 CRC32 */
    uint8_t     reserved[48];       /* 填充到 64 字节 */
} OTA_Slot1_t;

#pragma pack()  /* 恢复默认编译器对齐 */

/* ---------- 元数据操作 API ---------- */
void ota_slot0_init(OTA_Slot0_t *s);           /* 清空结构体并设置魔数和默认标志 */
int  ota_slot0_read(OTA_Slot0_t *s);           /* 从 Flash 读取 Slot0，0=成功，-1=魔数不匹配 */
int  ota_slot0_write(const OTA_Slot0_t *s);    /* 写入 Slot0 到 Flash（仅当槽位在擦除状态时有效） */
void ota_slot1_init(OTA_Slot1_t *s);           /* 清空结构体并设置魔数和默认分区 */
int  ota_slot1_read(OTA_Slot1_t *s);           /* 从 Flash 读取 Slot1 */
int  ota_slot1_write(const OTA_Slot1_t *s);    /* 写入 Slot1 到 Flash */
void ota_print_slots(const OTA_Slot0_t *s0, const OTA_Slot1_t *s1);  /* 串口打印双槽位内容（调试用） */

/******************************************************************************
 *  boot_jump_to_app —— 应用启动跳转函数
 *
 *  这是 Bootloader 的核心操作：从当前代码（Bootloader）跳转到目标 App。
 *  它直接操作 Cortex-M4 内核寄存器，不经过任何函数返回。
 *
 *  跳转步骤：
 *    1. 关全局中断（__disable_irq），停 SysTick
 *    2. 清空所有挂起的中断（Pending），防止 App 刚启动就误入旧中断
 *    3. 从目标地址读前 4 字节 → 主栈指针（MSP，Main Stack Pointer）
 *    4. 从目标地址读第 5~8 字节 → 复位向量（Reset_Handler 地址）
 *    5. 设置 MSP 到 App 的栈顶
 *    6. 设置 VTOR（向量表偏移寄存器）指向 App 的新向量表
 *    7. 函数指针跳转到 Reset_Handler
 *
 *  __attribute__((noreturn)) 告诉编译器该函数不返回，避免 "control reaches end"
 *  这类警告。
 */
void boot_jump_to_app(uint32_t app_addr) __attribute__((noreturn));

/* ---------- Bootloader 级系统初始化 ---------- */
void system_init(void);              /* 最小系统初始化：时钟 → 延时 → 串口(115200) → LED */
void SystemClock_Config(void);       /* 独立时钟树配置：HSE(12MHz) → PLL(336M) → SYSCLK(168MHz) */

#endif /* __MAIN_H */
