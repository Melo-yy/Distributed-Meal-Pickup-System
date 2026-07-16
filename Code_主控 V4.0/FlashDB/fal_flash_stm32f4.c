/*
 * fal_flash_stm32.c
 * FAL flash driver for STM32F4 standard version
 *
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_flash.h"

/* Flash sector address definitions */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* 16 KB */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* 16 KB */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* 16 KB */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* 16 KB */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* 64 KB */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* 128 KB */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* 128 KB */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* 128 KB */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* 128 KB */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* 128 KB */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* 128 KB */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* 128 KB */

/* Get sector number from address */
static uint32_t stm32_get_sector(uint32_t addr)
{
    if (addr < ADDR_FLASH_SECTOR_1)  return FLASH_Sector_0;
    else if (addr < ADDR_FLASH_SECTOR_2) return FLASH_Sector_1;
    else if (addr < ADDR_FLASH_SECTOR_3) return FLASH_Sector_2;
    else if (addr < ADDR_FLASH_SECTOR_4) return FLASH_Sector_3;
    else if (addr < ADDR_FLASH_SECTOR_5) return FLASH_Sector_4;
    else if (addr < ADDR_FLASH_SECTOR_6) return FLASH_Sector_5;
    else if (addr < ADDR_FLASH_SECTOR_7) return FLASH_Sector_6;
    else if (addr < ADDR_FLASH_SECTOR_8) return FLASH_Sector_7;
    else if (addr < ADDR_FLASH_SECTOR_9) return FLASH_Sector_8;
    else if (addr < ADDR_FLASH_SECTOR_10) return FLASH_Sector_9;
    else if (addr < ADDR_FLASH_SECTOR_11) return FLASH_Sector_10;
    else return FLASH_Sector_11;
}

/* FAL interface implementations */
static int read(long offset, uint8_t *buf, size_t size)
{
    long addr = stm32_onchip_flash.addr + offset;
    for (size_t i = 0; i < size; i++, buf++, addr++)
    {
        *buf = *(uint8_t *)addr;
    }
    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    long addr = stm32_onchip_flash.addr + offset;

    FLASH_Unlock();

    for (size_t i = 0; i < size; i++, addr++, buf++)
    {
        if (FLASH_ProgramByte(addr, *buf) != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return -1;
        }
        if (*(uint8_t *)addr != *buf)
        {
            FLASH_Lock();
            return -1;
        }
    }

    FLASH_Lock();
    return size;
}

static int erase(long offset, size_t size)
{
    uint32_t FirstSector = stm32_get_sector(stm32_onchip_flash.addr + offset);
    uint32_t LastSector  = stm32_get_sector(stm32_onchip_flash.addr + offset + size - 1);

    FLASH_Unlock();

    for (uint32_t sector = FirstSector; sector <= LastSector; sector += 8)
    {
        if (FLASH_EraseSector(sector, VoltageRange_3) != FLASH_COMPLETE)
        {
            FLASH_Lock();
            return -1;
        }
    }

    FLASH_Lock();
    return size;
}

/* FAL flash device definition */
const struct fal_flash_dev stm32_onchip_flash =
{
    .name       = "stm32_onchip",
    .addr       = 0x08000000,
    .len        = 512*1024,         /* 512KB Flash，适配 STM32F407VET6（原 1MB 声明对 VET6 无效） */
    .blk_size   = 128*1024,
    .ops        = {NULL, read, write, erase},
    .write_gran = 8
};
