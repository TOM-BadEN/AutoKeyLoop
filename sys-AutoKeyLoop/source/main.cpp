#include <switch.h>
#include <string.h>
#include <stdio.h>
#include "app.hpp"

// libnx fake heap initialization
extern "C" {

// 内部堆的大小（根据需要调整） 128KB
#define INNER_HEAP_SIZE 0x20000

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
  
  // 先初始化 setsys，获取版本后立即退出
  setsysInitialize();
  SetSysFirmwareVersion fw;
  if (R_SUCCEEDED(setsysGetFirmwareVersion(&fw)))
      hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
  setsysExit();
  
  fsInitialize();
  fsdevMountSdmc();
  hidInitialize();
  hiddbgInitialize();
  hidsysInitialize();
  pmdmntInitialize();
  pmshellInitialize();
}

void __appExit(void) {
  pmshellExit();
  pmdmntExit();     // 8. 进程管理服务（最后初始化的，最先退出）
  hidsysExit();     // 7. hidsys清理
  hiddbgExit();     // 6. hiddbg清理
  hidExit();        // 5. hid清理
  fsdevUnmountAll(); // 4. 卸载文件系统
  fsExit();         // 3. 文件系统服务
  smExit();         // 1. sm服务（最先初始化的，最后退出）
}

}

int main() {
  App app;
  app.Loop();
  return 0;
}
