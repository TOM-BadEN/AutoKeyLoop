#pragma once
#include <malloc.h>
#include <cstdio>
#include <tsl_utils.hpp>

class MemMonitor {
private:
    static inline size_t s_baseline = 0;

    // 静态占用（根据 ELF 段信息）
    static constexpr size_t TEXT_SIZE = 0x1271A0;  // 代码段 ~1.15MB
    static constexpr size_t DATA_SIZE = 0x4FD0;    // 已初始化数据 ~20KB
    static constexpr size_t BSS_SIZE  = 0x9130;    // 未初始化数据 ~36KB
    static constexpr size_t STACK_SIZE = 0x4000;   // 栈 ~16KB
    static constexpr size_t STATIC_OVERHEAD = TEXT_SIZE + DATA_SIZE + BSS_SIZE + STACK_SIZE;

    static size_t getAvailableHeap() {
        size_t heapConfig = static_cast<size_t>(ult::currentHeapSize);
        return (heapConfig > STATIC_OVERHEAD) ? (heapConfig - STATIC_OVERHEAD) : 0;
    }

    static void log_internal(const char* tag, size_t used, ssize_t diff, bool isBaseline) {
        size_t available = getAvailableHeap();
        size_t usedPercent = (available > 0) ? (used * 100 / available) : 0;
        
        FILE* f = fopen("sdmc:/config/KeyX/mem.log", "a");
        if (f) {
            if (isBaseline) {
                fprintf(f, "[%s] 已用=%zuKB/%zuKB (%zu%%) (基准)\n", 
                    tag, used / 1024, available / 1024, usedPercent);
            } else {
                const char* sign = (diff >= 0) ? "+" : "";
                fprintf(f, "[%s] 已用=%zuKB/%zuKB (%zu%%) (%s%zdKB)\n", 
                    tag, used / 1024, available / 1024, usedPercent, sign, diff / 1024);
            }
            fclose(f);
        }
    }

public:
    static void setBaseline(const char* tag) {
        struct mallinfo mi = mallinfo();
        s_baseline = mi.uordblks;
        log_internal(tag, mi.uordblks, 0, true);
    }

    static void log(const char* tag) {
        struct mallinfo mi = mallinfo();
        ssize_t diff = mi.uordblks - s_baseline;
        log_internal(tag, mi.uordblks, diff, false);
    }
    
    static void clear() {
        FILE* f = fopen("sdmc:/config/KeyX/mem.log", "w");
        if (f) fclose(f);
    }
};
