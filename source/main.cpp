#include <switch.h>
#include "../log/log.h"
#include <string.h>
#include <stdio.h>
#include "app.hpp"

// libnx fake heap initialization
extern "C" {

// 内部堆的大小（根据需要调整）
#define INNER_HEAP_SIZE 0x80000

// 系统模块不应使用applet相关功能
u32 __nx_applet_type = AppletType_None;

// 系统模块通常只需要使用一个文件系统会话
u32 __nx_fs_num_sessions = 1;

// Newlib堆配置函数（使malloc/free能够工作）
void __libnx_initheap(void) {
    static u8 inner_heap[INNER_HEAP_SIZE];
    extern void* fake_heap_start;
    extern void* fake_heap_end;

    // 配置newlib堆
    fake_heap_start = inner_heap;
    fake_heap_end   = inner_heap + sizeof(inner_heap);
}

void __appInit(void) {
  smInitialize();
  hidInitialize();
  fsInitialize();
  fsdevMountSdmc();
}

void __appExit(void) {
  fsdevUnmountAll();
  fsExit();
  hidExit();
  smExit();
}

}

int main() {
  MP::App app;
  app.Loop();
  return 0;
}
