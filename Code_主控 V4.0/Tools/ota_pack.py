#!/usr/bin/env python3
"""
OTA Firmware Packager —— OTA 固件打包脚本

用途：
  将 Keil/MDK 编译产出的 .bin 文件，附加 32 字节 OTA 头部，
  生成可直接上传到云端服务器的 .ota 文件。

用法示例：
  python ota_pack.py --bin WHEELTEC.bin --ver 4.1 --out WHEELTEC_V4_1.ota

头部格式（32 字节，小端序）：
  偏移  大小  字段          说明
  ─────────────────────────────────────
  0      4    MAGIC         固定魔数 0x4F54414D ("OTAM")
  4      4    VERSION       版本号（高 16 位=主版本，次 8 位=次版本，低 8 位=补丁）
  8      4    FW_LENGTH     固件数据长度（纯二进制，不含头部）
  12     4    FW_CRC32      固件数据的 CRC32 校验值
  16     4    TARGET_DEV    目标设备（0=主控，其他值=从控 ID）
  20     4    SEG_OFFSET    分片偏移（整包=0，分片下载时使用）
  24     8    RESERVED      保留字段

设计说明：
  与嵌入式 Bootloader 的 CRC32 算法（查表法，多项式 0xEDB88320）保持一致。
  这里使用 Python 标准库 zlib.crc32，与嵌入式实现二进制兼容。
"""

import struct
import zlib
import argparse
import os

# 魔数常量：必须与 Bootloader 和 App 的 OTA_SLOT0_MAGIC 一致
OTA_MAGIC = 0x4F54414D  # "OTAM"


def compute_crc32(data: bytes) -> int:
    """
    计算 CRC32 校验值（与嵌入式查表法完全兼容）

    参数：
      data: 二进制固件数据
    返回：
      32 位 CRC 值

    Python 的 zlib.crc32 与 STM32 上用查表法（多项式 0xEDB88320）
    计算的 CRC32 在二进制级别完全一致，无需额外转换。
    """
    return zlib.crc32(data) & 0xFFFFFFFF


def make_ota_header(magic: int, version: int, firmware_len: int,
                    firmware_crc: int, target: int = 0) -> bytes:
    """
    构造 32 字节 OTA 头部

    参数：
      magic:       魔数
      version:     版本号打包（由 parse_version 生成）
      firmware_len:  固件数据长度
      firmware_crc:  固件数据的 CRC32
      target:      目标设备（默认 0=主控）
    返回：
      32 字节二进制头部

    struct.pack 格式说明：
      <  = 小端序（Little-Endian），与 STM32F4 的字节序一致
      II = 两个 unsigned int（4 字节 × 2）
      后面依次类推
    """
    header = struct.pack('<IIIIIII',  # 7 个 unsigned int
                         magic,           # 0: 魔数
                         version,         # 4: 版本号
                         firmware_len,    # 8: 固件大小
                         firmware_crc,    # 12: CRC32
                         target,          # 16: 目标设备
                         0,               # 20: 分片偏移（整包）
                         0)               # 24: 保留
    header += b'\x00' * 4                 # 28~31: 填充到 32 字节
    return header


def parse_version(ver_str: str) -> int:
    """
    解析版本号字符串为嵌入式用的 uint32

    输入示例：
      "4.1"      → 0x00040100
      "4.1.0"    → 0x00040100
      "4"        → 0x00040000

    编码方式：
      高 16 位 = 主版本号（Major）
      次 8 位  = 次版本号（Minor）
      低 8 位  = 补丁号（Patch）

    与嵌入式中的 APP_VERSION_CURRENT 宏的编码方式完全一致。
    """
    parts = ver_str.split('.')
    major = int(parts[0])
    minor = int(parts[1]) if len(parts) > 1 else 0
    patch = int(parts[2]) if len(parts) > 2 else 0
    return (major << 16) | (minor << 8) | patch


def main():
    """
    主流程：
      1. 解析命令行参数
      2. 读取 .bin 文件
      3. 计算 CRC32
      4. 构造 32 字节头部
      5. 拼接头部 + 固件数据，写入 .ota 文件
    """
    parser = argparse.ArgumentParser(
        description='OTA Firmware Packager —— 固件打包工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例：
  python ota_pack.py --bin WHEELTEC.bin --ver 4.1 --out WHEELTEC_V4_1.ota
  python ota_pack.py --bin slave.bin --ver 1.0 --out slave.ota --target 1

上传到云端后，设备通过以下接口查询：
  GET /api/ota/check?v=00040000
  GET /api/ota/info?v=00040001
  GET /api/ota/download?offset=0&size=1024
        """)

    parser.add_argument('--bin', required=True,
                        help='输入的固件 .bin 文件路径（Keil 编译产物）')
    parser.add_argument('--ver', required=True,
                        help='版本号字符串，如 "4.1"、"4.1.0"')
    parser.add_argument('--out', required=True,
                        help='输出的 .ota 文件路径（上传到云端的文件）')
    parser.add_argument('--crc', type=lambda x: int(x, 0), default=None,
                        help='手动指定 CRC32（默认自动计算，用于复现构建场景）')
    parser.add_argument('--target', type=int, default=0,
                        help='目标设备 ID（0=主控，>=1=从控编号）')

    args = parser.parse_args()

    # ================================================================
    # 步骤 1：读取 .bin 固件文件
    # ================================================================
    if not os.path.exists(args.bin):
        print(f"错误：找不到文件 {args.bin}")
        return 1

    with open(args.bin, 'rb') as f:
        firmware = f.read()

    fw_len = len(firmware)
    print(f"固件大小：{fw_len} 字节 ({fw_len/1024:.1f} KB)")

    # ================================================================
    # 步骤 2：计算/获取 CRC32
    # ================================================================
    fw_crc = args.crc if args.crc is not None else compute_crc32(firmware)
    print(f"CRC32：0x{fw_crc:08X}")
    print(f"  云上 info 接口需要返回这个值：SIZE:{fw_len},CRC:0x{fw_crc:08X}")

    # ================================================================
    # 步骤 3：解析版本号
    # ================================================================
    version = parse_version(args.ver)
    print(f"版本号：{args.ver} → 0x{version:08X}")

    # ================================================================
    # 步骤 4：构造 32 字节 OTA 头部
    # ================================================================
    header = make_ota_header(OTA_MAGIC, version, fw_len, fw_crc, args.target)
    print(f"头部（{len(header)} 字节）：{header.hex()}")

    # ================================================================
    # 步骤 5：拼接输出
    # ================================================================
    with open(args.out, 'wb') as f:
        f.write(header)   # 32 字节头部
        f.write(firmware)  # 原始固件数据

    print(f"\n输出：{args.out}（共 {fw_len + len(header)} 字节）")
    print("请将此 .ota 文件上传到云服务器。")
    return 0


if __name__ == '__main__':
    exit(main())
