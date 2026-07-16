/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#define FAL_DEBUG 1
#define FAL_PART_HAS_TABLE_CFG

/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev stm32_onchip_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &stm32_onchip_flash,                                             \
}

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/*
 * FlashDB 分区表
 *
 * STM32F407VE 片内 Flash 共 512KB，分区如下：
 *   0x08000000  Bootloader (32KB)         — S0+S1，独立工程
 *   0x08008000  App A (224KB)             — S2~S5，主应用程序
 *   0x08040000  App B (128KB)             — S6，OTA 下载目标
 *   0x08060000  fdb_kvdb1 (128KB)         — S7，键值数据库
 *
 * kvdb1 存储：
 *   - 订单信息（order_t 结构体）
 *   - 设备-产品通道映射（device_id → product_id）
 *   - 派送信息（DeliveryInfo）
 *   - 系统配置参数
 *
 * 变更说明（适配 F407VE 的 512KB Flash）：
 *   1. 移除 fdb_tsdb1（时序数据库，代码中未使用）
 *   2. kvdb1 从原 512KB 偏移改为 384KB 偏移，大小缩减为 128KB
 *      — 原偏移 512KB 已超出 F407VE 的地址范围，写入实际无效
 */
#define FAL_PART_TABLE                                                                \
{                                                                                     \
    {FAL_PART_MAGIC_WORD, "fdb_kvdb1",    "stm32_onchip",   384*1024, 128*1024, 0},  \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
