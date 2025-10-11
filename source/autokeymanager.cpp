#include "autokeymanager.hpp"
#include <cstring>
#include "../log/log.h"


// 静态线程栈定义
alignas(0x1000) char AutoKeyManager::test_thread_stack[16 * 1024];

// 构造函数
AutoKeyManager::AutoKeyManager() {
    // 初始化线程状态标志
    thread_created = false;
    thread_running = false;
    
    // 清零线程对象
    memset(&test_thread, 0, sizeof(Thread));
    
    // 尝试创建测试线程
    // 根据 sys-autokeyloop.json 配置：优先级44，CPU核心3
    // 参数顺序：线程对象、线程函数、参数、栈地址、栈大小、优先级、CPU核心
    Result rc = threadCreate(&test_thread, test_thread_func, this, 
                           test_thread_stack, sizeof(test_thread_stack), 44, 3);
    
    if (R_SUCCEEDED(rc)) {
        thread_created = true;
        
        // 启动线程
        rc = threadStart(&test_thread);
        if (R_SUCCEEDED(rc)) {
            thread_running = true;
        }
    }
}

// 析构函数
AutoKeyManager::~AutoKeyManager() {
    // 停止线程运行
    if (thread_running) {
        thread_running = false;
    }
    
    // 等待线程结束并清理
    if (thread_created) {
        threadWaitForExit(&test_thread);
        Result close_result = threadClose(&test_thread);
        if (R_FAILED(close_result)) {
            // 线程关闭失败，但析构函数不能抛出异常，只能继续
        }
        thread_created = false;
    }
}

// 测试线程函数实现
void AutoKeyManager::test_thread_func(void* arg) {
    AutoKeyManager* manager = static_cast<AutoKeyManager*>(arg);
    
    // 简单的测试循环，每秒输出一次
    int counter = 0;
    while (manager->thread_running && counter < 10) {
        svcSleepThread(1000000000ULL); // 睡眠1秒 (1秒 = 1,000,000,000纳秒)
        counter++;
        log_info("AutoKeyManager测试线程运行中... 已运行 %d 秒", counter);
    }
    
    // 线程结束时设置标志
    manager->thread_running = false;
}