#pragma once
#include <switch.h>
#include <switch/services/hiddbg.h>
#include <mutex>

class AutoKeyManager {
private:
    // 连发线程相关变量
    Thread m_AutoKeyThread;
    alignas(0x1000) static char autokey_thread_stack[4 * 1024];
    bool m_ThreadCreated = false;
    bool m_ThreadRunning = false;
    bool m_ShouldExit = false;
    
    // 物理输入读取线程
    Thread m_InputReaderThread;
    alignas(0x1000) static char input_reader_thread_stack[4 * 1024];
    bool m_InputThreadCreated = false;
    bool m_InputThreadRunning = false;
    
    // 共享的物理输入状态（完整状态，包括摇杆）
    HidNpadHandheldState m_SharedPhysicalState;
    std::mutex m_InputMutex;
    
    // 配置参数互斥锁（保护连发配置的线程安全修改）
    std::mutex m_ConfigMutex;
    
    // HDLS工作缓冲区
    alignas(0x1000) static u8 hdls_work_buffer[0x1000];
    HiddbgHdlsSessionId m_HdlsSessionId;
    
    // HDLS状态列表（用于劫持现有控制器）
    HiddbgHdlsStateList m_StateList;
    bool m_HdlsInitialized;
    
    // 连发状态机
    bool m_AutoKeyIsPressed;
    u64 m_AutoKeyLastSwitchTime;
    
    // 连发按键池（白名单）- 只有这些按键允许连发
    u64 m_AutoKeyWhitelistMask;
    
    // 连发参数（可配置）
    u64 m_PressDurationNs;      // 按键按下持续时间（纳秒）
    u64 m_ReleaseDurationNs;    // 按键松开持续时间（纳秒）
    
    // 线程参数（固定）
    static const u64 UPDATE_INTERVAL_NS = 1000000ULL;         // 1ms线程更新

public:
    // 构造函数
    // @param buttons 白名单按键掩码 (u64)
    // @param presstime 按键按下持续时间（毫秒）
    // @param fireinterval 按键松开持续时间（毫秒）
    AutoKeyManager(u64 buttons, int presstime, int fireinterval);
    
    // 析构函数
    ~AutoKeyManager();
    
    // 动态更新配置参数（线程安全）
    // @param buttons 白名单按键掩码 (u64)
    // @param presstime 按键按下持续时间（毫秒）
    // @param fireinterval 按键松开持续时间（毫秒）
    void UpdateConfig(u64 buttons, int presstime, int fireinterval);
    
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
    
    // 时间窗口检测：仅在"按下周期"检测真松开（避免污染）
    bool CheckReleaseInWindow(u64 autokey_buttons);
};
