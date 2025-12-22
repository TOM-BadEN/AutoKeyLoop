#pragma once
#include <tesla.hpp>
#include <vector>
#include "macro_data.hpp"

class MacroSampler {
public:
    static void Prepare();              // 倒计时开始时调用：创建线程，线程空转等待
    static void Start();                // 倒计时结束时调用：通知线程开始采样
    static void Stop();                 // 停止采样
    static bool Save(u64 titleId, u64 comboMask = 0);  // 保存到文件，comboMask 用于移除末尾快捷键帧
    static void Cancel();               // 取消（倒计时期间按B）
    static const char* GetFilePath();   // 获取保存的文件路径
    
private:
    static void ThreadFunc(void* arg);
    static void SampleOneFrame();
    
    // 线程相关
    static Thread s_thread;
    static bool s_threadCreated;
    static bool s_shouldExit;
    static bool s_sampling;
    alignas(0x1000) static char s_threadStack[8 * 1024];
    
    // 采样数据
    static std::vector<MacroFrameV2> s_frames;
    static char s_filePath[128];
    static u32 s_totalSamples;  // 总采样次数
    static u64 s_startTick;     // 录制开始时间
    static u32 s_lastFrameMs;   // 上一帧时间(ms)
    static PadState s_padP1;        // pro 控制器
    static PadState s_padHandheld;  // 掌机模式
};
