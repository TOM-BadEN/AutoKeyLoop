
#include <stdio.h>
#include "log.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <switch.h>

// 日志系统全局变量
static Mutex log_mutex = 0;                                    // 日志互斥锁，确保多线程安全
#define LOG_FILE_PATH "/config/AutoKeyLoop/AutoKeyLoop.log"         // 日志文件路径
static FILE *log_file = NULL;                                  // 日志文件句柄
static bool g_log_enabled = false;                                  // 日志全局开关（默认关闭）

// 获取计时器时间字符串（从程序启动开始计时）
static char *cur_time() {
    static u64 start_time = 0;                                  // 程序启动时间
    static char timebuf[64];
    
    // 第一次调用时记录启动时间
    if (start_time == 0) {
        start_time = armGetSystemTick();
    }
    
    // 计算从启动到现在的时间差（纳秒）
    u64 current_time = armGetSystemTick();
    u64 elapsed_ns = armTicksToNs(current_time - start_time);
    
    // 转换为毫秒、秒、分钟
    u64 elapsed_ms = elapsed_ns / 1000000;                      // 纳秒转毫秒
    u64 total_seconds = elapsed_ms / 1000;                      // 毫秒转秒
    u64 ms = elapsed_ms % 1000;                                 // 剩余毫秒
    u64 seconds = total_seconds % 60;                           // 剩余秒数
    u64 minutes = (total_seconds / 60) % 60;                    // 剩余分钟
    
    // 格式化为 "MM:SS:mmm" 格式
    snprintf(timebuf, sizeof(timebuf), "[%02lu:%02lu:%03lu]",
             (unsigned long)minutes,                            // 分钟
             (unsigned long)seconds,                            // 秒数
             (unsigned long)ms);                                // 毫秒
    return timebuf;
}

// 设置日志开关（0=关闭，1=开启）
void log_set_enabled(bool enabled) {
    g_log_enabled = enabled;
}

// 日志写入核心函数
static void log_write(const char *level, const char *file, int line, const char *fmt, va_list args) {
    // 检查日志是否启用
    if (!g_log_enabled) return;
    
    mutexLock(&log_mutex);                                      // 加锁，确保线程安全
    if (!log_file) {
        log_file = fopen(LOG_FILE_PATH, "w");                   // 以写入模式打开日志文件，程序重启时重置内容
        if (!log_file) {
            mutexUnlock(&log_mutex);                            // 打开失败则解锁并返回
            return;
        }
    }
    // 只打印文件名最后20个字符，避免路径过长
    const char *short_file = file;
    size_t file_len = strlen(file);
    if (file_len > 20) {
        short_file = file + file_len - 20;                      // 截取文件名后15个字符
    }
    // 写入日志格式：时间 [文件名:行号] [级别] 消息内容
    fprintf(log_file, "%s [%s:%d] [%s] ", cur_time(), short_file, line, level);
    vfprintf(log_file, fmt, args);                              // 写入格式化的消息内容
    fprintf(log_file, "\n");                                   // 换行
    fflush(log_file);                                           // 立即刷新缓冲区到文件
    mutexUnlock(&log_mutex);                                    // 解锁
}

// INFO级别日志实现函数
void log_info_impl(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);                                        // 初始化可变参数列表
    log_write("INFO", file, line, fmt, args);                  // 调用核心写入函数
    va_end(args);                                               // 清理可变参数列表
}

// WARNING级别日志实现函数
void log_warning_impl(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_write("WARNING", file, line, fmt, args);
    va_end(args);
}

// ERROR级别日志实现函数
void log_error_impl(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_write("ERROR", file, line, fmt, args);
    va_end(args);
}

// DEBUG级别日志实现函数
void log_debug_impl(const char *file, int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_write("DEBUG", file, line, fmt, args);
    va_end(args);
}