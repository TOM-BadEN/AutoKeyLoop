#pragma once
#include <switch.h>
#include <functional>
#include <atomic>
#include <cstring>

namespace Thd {

    alignas(0x1000) inline char s_stack[32 * 1024];
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
    }
    
    inline void start(std::function<void()> task) {
        stop();
        s_task = task;
        s_done = false;
        memset(&s_thread, 0, sizeof(Thread));
        if (R_SUCCEEDED(threadCreate(&s_thread, threadFunc, nullptr, s_stack, sizeof(s_stack), 0x2C, -2))) {
            s_created = true;
            threadStart(&s_thread);
        }
    }
    
    inline bool isDone() { return s_done; }
}
