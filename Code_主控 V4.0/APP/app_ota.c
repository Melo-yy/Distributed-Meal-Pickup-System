/*
 * app_ota.c —— OTA 在线固件升级模块（App 侧）
 *
 * 职责：
 *   1. 定时轮询云端，检查是否有新固件版本
 *   2. 通过 4G 模块（HTTP）分片下载固件到 App B 扇区
 *   3. CRC32 完整性校验
 *   4. 写 Slot0 触发 Bootloader 切换
 *
 * 运行环境：FreeRTOS 独立任务（优先级低于取餐等业务任务）
 *
 * 与 Bootloader 的分工：
 *   本模块只负责"下载 + 校验 + 触发"。
 *   真正的"切换 + 回滚"逻辑在 Bootloader 中。
 */

#include "ota_meta.h"
#include "app_4g.h"       /* 复用 4G 模块的 HTTP 通信原语 */
#include "bsp_4g.h"       /* USART_4G_SendString 等硬件发送函数 */
#include "debug_config.h" /* 日志宏 DBG_INFO / DBG_ERROR / DBG_WARN */
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_flash.h"
#include "event_groups.h"

/* ======================================================================== */
/* 引用 4G 模块的全局缓冲区（定义在 app_4g.c 中）                            */
/*                                                                          */
/* http_rx_buffer：USART6 中断接收的原始数据，最大 MAX_PACKET_SIZE(1024) 字节  */
/* packet_index：当前有效接收字节数（ISR 中递增）                              */
/*                                                                          */
/* 注意：http_rx_buffer 也被 app_4g.c 本身的业务（订单同步）共用，            */
/*       所以在 OTA 下载期间要独占使用，不能同时做订单查询等操作。              */
/* ======================================================================== */
extern uint8_t http_rx_buffer[MAX_PACKET_SIZE];
extern uint16_t packet_index;

/******************************************************************************
 * CRC32 查表法实现（与 Bootloader 中的表一致）
 *
 * 两个独立副本（Bootloader 一份、App 一份）——避免跨模块依赖。
 * 可以抽成公共模块，但嵌入式项目中"适度冗余"比"过度抽象"更可靠。
 *
 * 多项式 0xEDB88320（逆向表示），与 zlib / Python binascii.crc32 兼容。
 *****************************************************************************/
static const uint32_t crc32_table[256] = {
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
 * CRC32_Compute（RAM 缓冲区）
 *  对新下载的固件数据做逐字节校验（调用前数据已被 flash_write 写入 Flash，
 *  但这里也提供一个纯内存版本，保留备用）。
 *****************************************************************************/
static uint32_t crc32_compute(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (uint32_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFUL;
}

/******************************************************************************
 * CRC32_Region（Flash 地址空间）
 *  直接对 App B 扇区的 Flash 地址做 CRC 校验，无需 RAM 中转。
 *  校验范围是 firmware_size 字节（通常 50~80KB），耗时约 50ms@168MHz。
 *****************************************************************************/
static uint32_t crc32_region(uint32_t addr, uint32_t length)
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
 *  Flash 底层操作（与 Bootloader 的 flash_write 功能相同）
 *
 *  这里重复实现而非调用 Bootloader 的函数，是因为：
 *    App 和 Bootloader 是独立的两个编译产物，不共享代码。
 *  但逻辑完全一致：解锁 → 逐字节编程 → 读回验证 → 重新上锁。
 *****************************************************************************/

/* Flash 控制器解锁 + 清错误标志 */
static void flash_unlock(void)
{
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

/******************************************************************************
 * 擦除 App B 扇区（Sector 6，128KB）
 *
 * 这是 OTA 下载前的关键步骤：
 *   把（可能的）旧固件和旧的 Slot0/Slot1 全清掉，回到全 1 状态。
 *   擦除后用 FLASH_EraseSector API，指定 FLASH_Sector_6。
 *
 * 注意：擦除过程中 Flash 不可读，如果代码在 Flash 中运行（即 XIP），
 *       擦除期间不能有中断或任务切换。但 FreeRTOS 任务可以在 Flash 中
 *       连续运行（代码已经预取到缓存），短时间的擦除不影响执行。
 *****************************************************************************/
static int flash_erase_app_b(void)
{
    flash_unlock();
    if (FLASH_EraseSector(FLASH_Sector_6, VoltageRange_3) != FLASH_COMPLETE) {
        FLASH_Lock();
        DBG_ERROR("[OTA] Erase sector 6 failed!\r\n");
        return -1;
    }
    FLASH_Lock();
    DBG_INFO("[OTA] Sector 6 erased\r\n");
    return 0;
}

/******************************************************************************
 * 逐字节写入 Flash
 *
 * 用于写入 Slot0 元数据（64 字节量级）。
 * 下载固件过程中的分片也使用此函数（每下载 1KB 调用一次）。
 *****************************************************************************/
static int flash_write_raw(uint32_t addr, const uint8_t *data, uint32_t len)
{
    flash_unlock();
    for (uint32_t i = 0; i < len; i++) {
        if (FLASH_ProgramByte(addr + i, data[i]) != FLASH_COMPLETE) {
            FLASH_Lock();
            return -1;
        }
        /* 读回验证（Read-After-Write Verification） */
        if (*(volatile uint8_t *)(addr + i) != data[i]) {
            FLASH_Lock();
            return -1;
        }
    }
    FLASH_Lock();
    return 0;
}

/******************************************************************************
 *  OTA 元数据读写（Slot0 / Slot1）
 *
 *  OTA_ReadSlot0 / OTA_ReadSlot1：
 *    直接从 Flash 地址 memcpy 到结构体，然后检查魔数。
 *    魔数不匹配返回 -1，调用方据此判断槽位是否有效。
 *
 *  OTA_WriteSlot0：
 *    写 Slot0 触发升级请求。这个调用发生在：
 *      固件下载完成 + CRC 校验通过 → 写 Slot0 → NVIC_SystemReset()
 *****************************************************************************/
int OTA_ReadSlot0(OTA_Slot0_t *s)
{
    memcpy(s, (const void *)OTA_SLOT0_ADDR, sizeof(OTA_Slot0_t));
    return (s->magic == OTA_SLOT0_MAGIC) ? 0 : -1;
}

int OTA_WriteSlot0(const OTA_Slot0_t *s)
{
    return flash_write_raw(OTA_SLOT0_ADDR, (const uint8_t *)s, sizeof(OTA_Slot0_t));
}

int OTA_ReadSlot1(OTA_Slot1_t *s)
{
    memcpy(s, (const void *)OTA_SLOT1_ADDR, sizeof(OTA_Slot1_t));
    return (s->magic == OTA_SLOT1_MAGIC) ? 0 : -1;
}

/******************************************************************************
 * OTA_Init —— OTA 模块初始化（在 start_task 中调用）
 *
 * 读取 Slot1 获取当前启动区域信息，仅做日志记录，不做写入操作。
 * 保持只读，避免违反"每擦除周期只写一次"的约束。
 *****************************************************************************/
void OTA_Init(void)
{
    OTA_Slot1_t s1;

    if (OTA_ReadSlot1(&s1) == 0) {
        DBG_INFO("[OTA] Running from region %s (v=0x%08lX)\r\n",
               s1.active_region == ACTIVE_REGION_A ? "A" : "B",
               s1.app_a_version);
    } else {
        DBG_INFO("[OTA] First boot (no Slot1 metadata)\r\n");
    }
}

/******************************************************************************
 *  HTTP 通信辅助函数
 *
 *  ota_http_get 使用二进制信号量（xSemaphore4G_RX）替代轮询 vTaskDelay。
 *  USART6 ISR 每收到一个字节就释放信号量，OTA 任务通过 xSemaphoreTake 阻塞等待。
 *  空闲时任务不消耗 CPU，体现 RTOS 事件驱动优势。
 *****************************************************************************/

/* 清空接收索引 */
static void ota_rx_clear(void)
{
    packet_index = 0;
}

/*
 * ota_http_get —— 发送 HTTP GET 请求并等待完整响应
 *
 * path:       URL 路径
 * timeout_ms: 首字节超时
 * 返回:       成功=接收字节数，失败=-1
 */
static int ota_http_get(const char *path, uint32_t timeout_ms)
{
    ota_rx_clear();
    USART_4G_SendString(path);

    /*
     * Phase 1：用信号量等第一个字节
     * 之前用 vTaskDelay(5ms) 轮询，现在 ISR 驱动，不空耗 CPU
     */
    if (xSemaphoreTake(xSemaphore4G_RX, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        DBG_ERROR("[OTA] HTTP GET timeout: %s\r\n", path);
        ota_rx_clear();
        return -1;
    }

    /*
     * Phase 2：继续收后续字节，50ms 空闲判定结束
     */
    uint32_t idle_time = 0;
    uint16_t last_rx = packet_index;

    while (idle_time < 100) {
        if (xSemaphoreTake(xSemaphore4G_RX, pdMS_TO_TICKS(10)) == pdTRUE) {
            last_rx = packet_index;
            idle_time = 0;
        } else {
            idle_time += 10;
            if (idle_time >= 50) break;
        }
    }

    return (int)packet_index;
}

/******************************************************************************
 *  云端版本查询
 *
 *  HTTP GET：/api/ota/check?v=00040000
 *  响应约定： "NEW:00040001" → 有新版本
 *            "CURRENT"      → 已是最新
 *****************************************************************************/
static int ota_check_version(uint32_t *new_version)
{
    char req[64];
    snprintf(req, sizeof(req), "/api/ota/check?v=%08lX", APP_VERSION_CURRENT);

    int resp_len = ota_http_get(req, 5000);
    if (resp_len <= 0) return -1;

    http_rx_buffer[resp_len < MAX_PACKET_SIZE - 1 ? resp_len : MAX_PACKET_SIZE - 1] = '\0';

    if (strstr((char *)http_rx_buffer, "NEW:") != NULL) {
        unsigned long ver = 0;
        if (sscanf((char *)http_rx_buffer, "NEW:%lx", &ver) == 1) {
            *new_version = (uint32_t)ver;
            return 1;
        }
    }
    return 0;
}

/******************************************************************************
 *  下载单个固件分片
 *
 *  请求：GET /api/ota/download?offset=0&size=1024
 *  收到后直接写 Flash，不缓存整包，RAM 占用仅 1KB
 *****************************************************************************/
static int ota_download_chunk(uint32_t offset, uint32_t size)
{
    char req[80];
    snprintf(req, sizeof(req), "/api/ota/download?offset=%lu&size=%lu", offset, size);

    int resp_len = ota_http_get(req, OTA_CHUNK_TIMEOUT_MS);
    if (resp_len <= 0) return -1;

    if (flash_write_raw(APP_B_ADDR + offset, http_rx_buffer, resp_len) != 0) {
        return -1;
    }
    return resp_len;
}

/******************************************************************************
 * OTA_Task —— OTA 升级主循环（FreeRTOS 独立任务）
 *
 * 状态机：
 *   IDLE → CHECK_VERSION → GET_FW_INFO → ERASE → DOWNLOAD_CHUNKS
 *        → CRC_VERIFY → WRITE_SLOT0 → REBOOT
 *
 * 任何一步失败 → 回到 IDLE 等下次周期重试
 *****************************************************************************/
void OTA_Task(void *pvParameters)
{
    uint32_t new_version = 0;
    uint32_t firmware_size = 0;
    uint32_t expected_crc = 0;

    DBG_INFO("[OTA] Task started, interval=%lu s\r\n", OTA_CHECK_INTERVAL_MS / 1000);

    /* 等待 4G 模块就绪后开始（演示事件组用法） */
    xEventGroupWaitBits((EventGroupHandle_t)xStartupEventGroup, BIT_4G_READY,
                        pdFALSE, pdTRUE, pdMS_TO_TICKS(10000));
    DBG_INFO("[OTA] 4G ready, starting main loop\r\n");

    for (;;) {
        /* ================================================================ */
        /* 状态 1：版本检查                                                */
        /* ================================================================ */
        int ret = ota_check_version(&new_version);
        if (ret <= 0) {
            vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
            continue;
        }

        DBG_INFO("[OTA] New version: 0x%08lX\r\n", new_version);

        /* ================================================================ */
        /* 状态 2：获取固件信息（总大小 + CRC）                             */
        /* ================================================================ */
        {
            char req[64];
            snprintf(req, sizeof(req), "/api/ota/info?v=%08lX", new_version);
            if (ota_http_get(req, 5000) > 0) {
                http_rx_buffer[MAX_PACKET_SIZE - 1] = '\0';
                sscanf((char *)http_rx_buffer, "SIZE:%lu,CRC:%lx",
                       (unsigned long *)&firmware_size,
                       (unsigned long *)&expected_crc);
                DBG_INFO("[OTA] FW: %lu bytes, CRC=0x%08lX\r\n", firmware_size, expected_crc);

                if (firmware_size == 0 || firmware_size > APP_B_SIZE - 4096) {
                    DBG_ERROR("[OTA] Bad size: %lu\r\n", firmware_size);
                    vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
                    continue;
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
                continue;
            }
        }

        /* ================================================================ */
        /* 状态 3：擦除 App B 扇区                                          */
        /* ================================================================ */
        if (flash_erase_app_b() != 0) {
            vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
            continue;
        }

        /* ================================================================ */
        /* 状态 4：逐分片下载固件                                           */
        /* ================================================================ */
        uint32_t offset = 0;
        int fail = 0;
        while (offset < firmware_size) {
            uint32_t chunk = firmware_size - offset;
            if (chunk > OTA_CHUNK_SIZE) chunk = OTA_CHUNK_SIZE;

            if (ota_download_chunk(offset, chunk) <= 0) {
                DBG_ERROR("[OTA] Chunk fail at offset %lu\r\n", offset);
                fail = 1;
                break;
            }
            offset += chunk;
            DBG_INFO("[OTA] %lu/%lu (%lu%%)\r\n", offset, firmware_size,
                   offset * 100 / firmware_size);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (fail) {
            vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
            continue;
        }

        /* ================================================================ */
        /* 状态 5：CRC32 全量校验                                           */
        /* ================================================================ */
        uint32_t actual = crc32_region(APP_B_ADDR, firmware_size);
        if (actual != expected_crc) {
            DBG_ERROR("[OTA] CRC fail: want 0x%08lX, got 0x%08lX\r\n", expected_crc, actual);
            vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
            continue;
        }
        DBG_INFO("[OTA] CRC OK 0x%08lX\r\n", actual);

        /* ================================================================ */
        /* 状态 6：写 Slot0 触发 Bootloader 切换                           */
        /* ================================================================ */
        OTA_Slot0_t s0;
        memset(&s0, 0, sizeof(s0));
        s0.magic = OTA_SLOT0_MAGIC;
        s0.to_b_flag = SLOT0_FLAG_TO_B;
        s0.app_b_version = new_version;
        s0.app_b_crc = expected_crc;

        if (OTA_WriteSlot0(&s0) == 0) {
            DBG_INFO("[OTA] Done! Rebooting...\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            NVIC_SystemReset();
        }

        DBG_ERROR("[OTA] Slot0 write failed!\r\n");
        vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
    }
}
