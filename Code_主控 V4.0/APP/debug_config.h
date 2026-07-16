#ifndef __DEBUG_CONFIG_H
#define __DEBUG_CONFIG_H

// 调试级别定义
#define DEBUG_LEVEL_NONE    0   // 不输出任何调试信息
#define DEBUG_LEVEL_ERROR   1   // 只输出错误信息
#define DEBUG_LEVEL_WARN    2   // 输出错误和警告信息
#define DEBUG_LEVEL_INFO    3   // 输出错误、警告和一般信息
#define DEBUG_LEVEL_DEBUG   4   // 输出所有调试信息

//#define PRODUCTION_BUILD   1

// 当前调试级别配置
#ifdef PRODUCTION_BUILD
    // 生产版本：只输出错误信息
    #define CURRENT_DEBUG_LEVEL   DEBUG_LEVEL_ERROR
#else
    // 开发版本：输出所有调试信息
    #define CURRENT_DEBUG_LEVEL   DEBUG_LEVEL_DEBUG
#endif

// 条件编译宏定义
#if CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
    #define DBG_ERROR(fmt, ...)     printf("[ERROR] " fmt, ##__VA_ARGS__)
#else
    #define DBG_ERROR(fmt, ...)
#endif

#if CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_WARN
    #define DBG_WARN(fmt, ...)      printf("[WARN]  " fmt, ##__VA_ARGS__)
#else
    #define DBG_WARN(fmt, ...)
#endif

#if CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_INFO
    #define DBG_INFO(fmt, ...)      printf("[INFO]  " fmt, ##__VA_ARGS__)
#else
    #define DBG_INFO(fmt, ...)
#endif

#if CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG
    #define DBG_DEBUG(fmt, ...)     printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
    #define DBG_DEBUG(fmt, ...)
#endif

#endif // __DEBUG_CONFIG_H
