#ifndef __OTA_META_H
#define __OTA_META_H

#include <stdint.h>
#include <string.h>

/******************************************************************************
 *  OTA 模块全局定义（App 侧与 Bootloader 共享的地址和数据结构）
 *
 *  这个头文件同时被 App 和 Bootloader 引用（或维护两个内容一致的副本），
 *  保证双方对 Flash 分区布局、槽位地址、结构体布局的理解完全一致。
 *
 *  一致性至关重要：
 *    如果 App 把 Slot0 写到一个地址，Bootloader 却从另一个地址读，
 *    升级就永远不会被触发。
 *****************************************************************************/

/* STM32F407VE: 片内 Flash 512KB */
#define FLASH_BASE_ADDR              0x08000000UL

/* Bootloader: S0+S1 = 32KB */
#define BOOTLOADER_SIZE              (32UL * 1024)

/* App A（当前应用程序）：S2~S5 = 224KB */
#define APP_A_ADDR                   (FLASH_BASE_ADDR + BOOTLOADER_SIZE)
#define APP_A_SIZE                   (224UL * 1024)

/* App B（OTA 下载目标）：S6 = 128KB */
#define APP_B_ADDR                   0x08040000UL
#define APP_B_SIZE                   (128UL * 1024)

/*
 * 双槽位元数据地址（位于 App B 扇区末尾）
 *
 * 物理约束：
 *   Flash 编程只能 1→0，只有扇区擦除才能 0→1。
 *   两个槽位在不同地址，各自在每次擦除周期内只写一次 → 无冲突。
 *
 * 写入时机：
 *   - App 下载前擦除 S6 → 两个槽位变为全 1
 *   - 下载完成后写 Slot0（触发）
 *   - Bootloader 验证后写 Slot1（固化决策）
 */
#define OTA_SLOT0_ADDR               0x0805F000UL    /* Slot0：App 写触发生升级请求 */
#define OTA_SLOT1_ADDR               0x0805F100UL    /* Slot1：Bootloader 写记录决策 */
#define OTA_SLOT_SIZE                64              /* 每槽 64 字节 */

/* ----- Slot0 魔数及标志 ----- */
#define OTA_SLOT0_MAGIC              0x4F54414DUL   /* "OTAM" */
#define SLOT0_FLAG_NORMAL            0x00            /* 无升级请求 */
#define SLOT0_FLAG_TO_B              0x5A            /* 请求切换到 B 区 */

/* ----- Slot1 魔数及标志 ----- */
#define OTA_SLOT1_MAGIC              0x4F544156UL   /* "OTAV"（已验证） */
#define ACTIVE_REGION_A              0
#define ACTIVE_REGION_B              1

/* ----- 当前运行版本号（每次 Release 前修改） ----- */
#define APP_VERSION_MAJOR            4
#define APP_VERSION_MINOR            0
#define APP_VERSION_PATCH            0
#define APP_VERSION_CURRENT          ((uint32_t)((APP_VERSION_MAJOR << 16) | \
                                                 (APP_VERSION_MINOR << 8)  | \
                                                 APP_VERSION_PATCH))

/* ----- OTA 下载参数 ----- */
#define OTA_CHUNK_SIZE               1024            /* 每片 1KB，兼顾 4G 模块吞吐与 Flash 编程速度 */
#define OTA_TASK_PRIO                3               /* FreeRTOS 任务优先级（低于业务任务） */
#define OTA_TASK_STK_SIZE            512             /* 任务栈大小（字节） */
#define OTA_CHECK_INTERVAL_MS        (60 * 1000)    /* 每 60 秒检查一次更新（可配置） */
#define OTA_CHUNK_TIMEOUT_MS         10000           /* 每片下载超时 10 秒 */

#pragma pack(1)  /* 1 字节对齐，保证跨编译器编译时结构体布局一致 */

/*
 * Slot0 结构体 —— App → Bootloader
 *
 * App 下载并验证完新固件后填写此结构体并写入 Flash。
 * Bootloader 在下次启动时读取，决定是否切到 B 区。
 */
typedef struct {
    uint32_t    magic;              /* 魔数 OTA_SLOT0_MAGIC，校验槽位有效性 */
    uint8_t     to_b_flag;          /* SLOT0_FLAG_NORMAL(无操作) / TO_B(请切换) */
    uint8_t     reserved1[3];       /* 对齐填充 */
    uint32_t    app_b_version;      /* 新固件的语义版本号（便于云端比对和日志） */
    uint32_t    app_b_crc;          /* 新固件的 CRC32，供 Bootloader 完整性校验 */
    uint8_t     reserved[48];       /* 预留 48 字节（可扩展签名、时间戳等） */
} OTA_Slot0_t;

/*
 * Slot1 结构体 —— Bootloader → App
 *
 * Bootloader 验证 App B 通过后写入，App 在 OTA_Init() 中读取。
 * 主要用于确认当前是从哪个分区启动的、运行的版本号是多少。
 */
typedef struct {
    uint32_t    magic;              /* 魔数 OTA_SLOT1_MAGIC */
    uint8_t     active_region;      /* 当前实际启动的区域：A(0) 或 B(1) */
    uint8_t     boot_attempts;      /* 启动尝试计数（预留防反复复位） */
    uint8_t     reserved1[2];
    uint32_t    app_a_version;      /* 当前运行固件的版本号 */
    uint32_t    app_a_crc;          /* 当前运行固件的 CRC32 */
    uint8_t     reserved[48];
} OTA_Slot1_t;
#pragma pack()

/* ----- 函数声明 ----- */
void OTA_Init(void);                     /* 读取 Slot1 信息，确认当前运行环境 */
int  OTA_ReadSlot0(OTA_Slot0_t *s);      /* 从 Flash 读 Slot0 */
int  OTA_WriteSlot0(const OTA_Slot0_t *s); /* 写 Slot0 到 Flash */
int  OTA_ReadSlot1(OTA_Slot1_t *s);      /* 从 Flash 读 Slot1 */
void OTA_Task(void *pvParameters);       /* FreeRTOS 任务：OTA 升级主循环 */

/* ===================== 启动同步事件组 ===================== */
/* EventGroupHandle_t 在 event_groups.h 中定义，包含前必须先包含 FreeRTOS.h。
   此处用 void* 做前向声明，使用时在 .c 文件中自行强转。 */
extern void* xStartupEventGroup;       /* 实际类型 EventGroupHandle_t */
#define BIT_CAN_READY          (1 << 0)   /* CAN 总线初始化完成 */
#define BIT_4G_READY           (1 << 1)   /* 4G 模块初始化完成 */
#define BIT_FLASHDB_READY      (1 << 2)   /* FlashDB 初始化完成 */
#define BIT_4G_BUSINESS_READY  (1 << 3)   /* 4G 业务就绪：配货单/订单下载完成 */
#define BIT_ALL_SYSTEMS        (BIT_CAN_READY | BIT_4G_READY | BIT_FLASHDB_READY | BIT_4G_BUSINESS_READY)

#endif /* __OTA_META_H */
