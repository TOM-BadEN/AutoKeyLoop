#pragma once
#include <switch.h>
#include <switch/services/hiddbg.h>
#include <mutex>

class AutoKeyManager {
private:
    // 连发线程相关变量
    Thread autokey_thread;
    alignas(0x1000) static char autokey_thread_stack[4 * 1024];
    bool thread_created = false;
    bool thread_running = false;
    bool should_exit = false;
    
    // 物理输入读取线程
    Thread input_reader_thread;
    alignas(0x1000) static char input_reader_thread_stack[4 * 1024];
    bool input_thread_created = false;
    bool input_thread_running = false;
    
    // 共享的物理输入状态（完整状态，包括摇杆）
    HidNpadHandheldState shared_physical_state;
    std::mutex input_mutex;
    
    // HDLS工作缓冲区
    alignas(0x1000) static u8 hdls_work_buffer[0x1000];
    HiddbgHdlsSessionId hdls_session_id;
    
    // HDLS状态列表（用于劫持现有控制器）
    HiddbgHdlsStateList state_list;
    bool hdls_initialized;
    
    // 连发状态机
    bool autokey_is_pressed;
    u64 autokey_last_switch_time;
    
    // 连发按键池（白名单）- 只有这些按键允许连发
    u64 autokey_whitelist_mask;
    
    // 连发参数（参考pad-macro项目）
    static const u64 FIRE_INTERVAL_NS = 100000000ULL;         // 100ms切换间隔（按下/松开）
    static const u64 UPDATE_INTERVAL_NS = 1000000ULL;         // 1ms线程更新

public:
    // 构造函数
    AutoKeyManager();
    
    // 析构函数
    ~AutoKeyManager();
    
private:
    // 连发线程函数
    static void autokey_thread_func(void* arg);
    
    // 连发处理函数
    void ProcessAutoKey();
    
    // 劫持并修改控制器状态
    void HijackAndModifyState();
    
    // 物理输入读取线程函数
    static void input_reader_thread_func(void* arg);
    
    // 物理输入读取循环
    void ProcessInputReading();
    
    // 读取物理输入（内部使用，返回完整状态）
    void ReadPhysicalInput(HidNpadHandheldState* out_state);
    
    // 获取共享的物理输入（线程安全）
    void GetSharedPhysicalState(HidNpadHandheldState* out_state);
};
