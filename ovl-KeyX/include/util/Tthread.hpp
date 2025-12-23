#pragma once
#include <switch.h>
#include <functional>
#include <atomic>
#include <cstring>

namespace Thd {

    constexpr size_t STACK_SIZE = 32 * 1024;  // 32KB 栈空间
    inline Thread s_thread;
    inline std::function<void()> s_task;
    inline std::atomic<bool> s_done{true};
    inline bool s_created = false;
    
    inline void threadFunc(void*) {
        if (s_task) s_task();
        s_done = true;
    }
    
    inline void stop() {
        if (s_created) {
            threadWaitForExit(&s_thread);
            threadClose(&s_thread);
            s_created = false;
        }
        s_task = nullptr;  // 清理 lambda，释放捕获的变量
    }
    
    inline void start(std::function<void()> task) {
        stop();
        s_task = task;
        s_done = false;
        memset(&s_thread, 0, sizeof(Thread));
        if (R_SUCCEEDED(threadCreate(&s_thread, threadFunc, nullptr, nullptr, STACK_SIZE, 0x2C, -2))) {
            s_created = true;
            threadStart(&s_thread);
        }
    }
    
    inline bool isDone() { return s_done; }
}
