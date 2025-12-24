#pragma once
#include <malloc.h>
#include <cstdio>

class MemMonitor {
public:
    static constexpr size_t TOTAL_HEAP = 4 * 1024 * 1024;  // 4MB 总堆
    static constexpr size_t NRO_SIZE = 1500 * 1024;        // ~1.46MB NRO
    
    static void log(const char* tag) {
        struct mallinfo mi = mallinfo();
        size_t available = TOTAL_HEAP - NRO_SIZE;  // 可用堆
        size_t remaining = (mi.arena < available) ? (available - mi.arena) : 0;
        
        FILE* f = fopen("sdmc:/config/KeyX/mem.log", "a");
        if (f) {
            fprintf(f, "[%s] 堆已用=%zuKB 堆剩余=%zuKB (arena=%zuKB)\n",
                tag, mi.uordblks/1024, remaining/1024, mi.arena/1024);
            fclose(f);
        }
    }
};