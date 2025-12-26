#include "macro_sampler.hpp"
#include <ultra.hpp>
#include <time.h>

namespace {
    constexpr u64 SAMPLE_INTERVAL_NS = 8333333ULL;  // ~120Hz (8.33ms)
    constexpr u64 WAIT_INTERVAL_NS = 1000000ULL;     // 等待时 1ms 检查一次
    constexpr u64 STICK_PSEUDO_MASK = 0xFF0000ULL;   // 摇杆伪按键掩码
    
    // 比较两帧是否相同
    bool isSameFrame(const MacroFrameV2& a, const MacroFrameV2& b) {
        return a.keysHeld == b.keysHeld &&
               a.leftX == b.leftX && a.leftY == b.leftY &&
               a.rightX == b.rightX && a.rightY == b.rightY;
    }
    
}

// 静态成员定义
Thread MacroSampler::s_thread;
bool MacroSampler::s_threadCreated = false;
std::atomic<bool> MacroSampler::s_shouldExit{false};
std::atomic<bool> MacroSampler::s_sampling{false};
std::vector<MacroFrameV2> MacroSampler::s_frames;
char MacroSampler::s_filePath[128] = {};
u32 MacroSampler::s_totalSamples = 0;
u64 MacroSampler::s_startTick = 0;
u32 MacroSampler::s_lastFrameMs = 0;
PadState MacroSampler::s_padP1 = {};
PadState MacroSampler::s_padHandheld = {};

// 采样一帧
void MacroSampler::SampleOneFrame() {
    // 计算时间
    u32 elapsedMs = armTicksToNs(armGetSystemTick() - s_startTick) / 1000000;
    u32 frameDuration = elapsedMs - s_lastFrameMs;
    
    // 读取 HID 输入（同时读取 pro 和 Handheld，合并输入）
    padUpdate(&s_padP1);
    padUpdate(&s_padHandheld);
    
    u64 keysHeld = padGetButtons(&s_padP1) | padGetButtons(&s_padHandheld);
    
    // 摇杆优先使用 Handheld，否则用 pro
    HidAnalogStickState leftStick_h = padGetStickPos(&s_padHandheld, 0);
    HidAnalogStickState rightStick_h = padGetStickPos(&s_padHandheld, 1);
    bool handheldHasStick = (leftStick_h.x != 0 || leftStick_h.y != 0 || rightStick_h.x != 0 || rightStick_h.y != 0);
    
    HidAnalogStickState leftStick = handheldHasStick ? leftStick_h : padGetStickPos(&s_padP1, 0);
    HidAnalogStickState rightStick = handheldHasStick ? rightStick_h : padGetStickPos(&s_padP1, 1);
    
    MacroFrameV2 frame;
    frame.durationMs = frameDuration;
    frame.keysHeld = keysHeld & ~STICK_PSEUDO_MASK;
    frame.leftX = leftStick.x;
    frame.leftY = leftStick.y;
    frame.rightX = rightStick.x;
    frame.rightY = rightStick.y;
    
    // 合并相同帧或添加新帧
    s_totalSamples++;
    if (!s_frames.empty() && isSameFrame(s_frames.back(), frame)) s_frames.back().durationMs += frameDuration;
    else s_frames.push_back(frame);
    s_lastFrameMs = elapsedMs;
}

// 线程函数
void MacroSampler::ThreadFunc(void* arg) {
    while (!s_shouldExit) {
        if (s_sampling) {
            SampleOneFrame();
            svcSleepThread(SAMPLE_INTERVAL_NS);
        } else {
            svcSleepThread(WAIT_INTERVAL_NS);
        }
    }
}

// 准备（倒计时开始时调用）
void MacroSampler::Prepare() {
    // 清理旧数据并预分配（避免多次扩容导致内存碎片化）
    s_frames.clear();
    s_frames.reserve(1024);  // 预分配 ~32KB
    s_filePath[0] = '\0';
    s_totalSamples = 0;
    s_startTick = 0;
    s_lastFrameMs = 0;
    s_sampling = false;
    s_shouldExit = false;
    
    // 初始化 pad（同时支持 pro 和 Handheld）
    padInitialize(&s_padP1, HidNpadIdType_No1);
    padInitialize(&s_padHandheld, HidNpadIdType_Handheld);
    padUpdate(&s_padP1);
    padUpdate(&s_padHandheld);
    
    // 创建线程
    memset(&s_thread, 0, sizeof(Thread));
    Result rc = threadCreate(&s_thread, ThreadFunc, nullptr, nullptr, 4 * 1024, 44, -2);
    if (R_SUCCEEDED(rc)) {
        s_threadCreated = true;
        threadStart(&s_thread);
    }
}

// 开始采样（倒计时结束时调用）
void MacroSampler::Start() {
    s_startTick = armGetSystemTick();
    s_sampling = true;
}

// 停止采样
void MacroSampler::Stop() {
    s_sampling = false;
    s_shouldExit = true;
    if (s_threadCreated) {
        threadWaitForExit(&s_thread);
        threadClose(&s_thread);
        s_threadCreated = false;
    }
}

// 取消录制
void MacroSampler::Cancel() {
    Stop();
    s_frames.clear();
    s_frames.shrink_to_fit();
}

// 保存到文件
bool MacroSampler::Save(u64 titleId, u64 comboMask) {
    if (s_frames.empty()) return false;
    // 移除末尾快捷键帧
    while (!s_frames.empty() && (s_frames.back().keysHeld & comboMask)) s_frames.pop_back();
    // 移除末尾无动作帧
    while (!s_frames.empty()) {
        auto& f = s_frames.back();
        if (f.keysHeld == 0 && f.leftX == 0 && f.leftY == 0 && f.rightX == 0 && f.rightY == 0) s_frames.pop_back();
        else break;
    }
    if (s_frames.empty()) return false;
    
    // 构造文件头
    MacroHeader header;
    memcpy(header.magic, "KEYX", 4);
    header.version = 2;
    header.frameRate = s_lastFrameMs ? (s_totalSamples * 1000 / s_lastFrameMs) : 0;
    header.titleId = titleId;
    header.frameCount = s_frames.size();
    
    // 生成目录
    char dirPath[64];
    sprintf(dirPath, "sdmc:/config/KeyX/macros/%016lX", titleId);
    ult::createDirectory(dirPath);
    
    // 生成文件名
    int suffix = 1;
    time_t now = time(nullptr);
    struct tm tmNow;
    localtime_r(&now, &tmNow);
    sprintf(s_filePath, "%s/%02d%02d_%02d_%02d_%02d.macro", dirPath, tmNow.tm_mon + 1, tmNow.tm_mday, tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
    while (ult::isFile(s_filePath)) {
        sprintf(s_filePath, "%s/%02d%02d_%02d_%02d_%02d_%d.macro", dirPath, tmNow.tm_mon + 1, tmNow.tm_mday, tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec, suffix++);
    }
    
    // 写入文件
    FILE* fp = fopen(s_filePath, "wb");
    if (!fp) return false;
    fwrite(&header, sizeof(header), 1, fp);
    fwrite(s_frames.data(), sizeof(MacroFrameV2), s_frames.size(), fp);
    fclose(fp);
    s_frames.clear(); 
    s_frames.shrink_to_fit();
    return true;
}

// 获取文件路径
const char* MacroSampler::GetFilePath() {
    return s_filePath;
}
