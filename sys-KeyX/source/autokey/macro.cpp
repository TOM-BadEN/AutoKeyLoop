#include "macro.hpp"
#include "minIni.h"
#include <cstdio>

// 常量定义
static constexpr u64 STOP_COOLDOWN_NS = 200000000ULL;        // 200ms 停止后延迟
static constexpr u64 LONG_PRESS_THRESHOLD_NS = 500000000ULL; // 500ms 长按阈值

// 构造函数
Macro::Macro(const char* macroCfgPath) {
    LoadConfig(macroCfgPath);
}

// 加载配置
void Macro::LoadConfig(const char* macroCfgPath) {
    m_Macros.clear();
    int macroCount = ini_getl("MACRO", "macroCount", 0, macroCfgPath);
    for (int i = 1; i <= macroCount; i++) {
        MacroEntry entry;
        char pathKey[32];
        sprintf(pathKey, "macro_path_%d", i);
        ini_gets("MACRO", pathKey, "", entry.MacroFilePath, sizeof(entry.MacroFilePath), macroCfgPath);
        char comboKey[32];
        sprintf(comboKey, "macro_combo_%d", i);
        char comboStr[32];
        ini_gets("MACRO", comboKey, "0", comboStr, sizeof(comboStr), macroCfgPath);
        entry.combo = strtoull(comboStr, nullptr, 10);
        if (entry.combo != 0 && entry.MacroFilePath[0] != '\0') m_Macros.push_back(entry);
    }
}

// 核心函数：处理输入
void Macro::Process(ProcessResult& result) {
    result.event = DetermineEvent(result.buttons);
    // 根据事件执行对应操作
    switch (result.event) {
        case FeatureEvent::IDLE:
        case FeatureEvent::PAUSED:
            return;
        case FeatureEvent::STARTING:
            MacroStarting();
            return;
        case FeatureEvent::Macro_EXECUTING:
            MacroExecuting(result);
            return;
        case FeatureEvent::FINISHING:
            MacroFinishing();
            return;
        default:
            return;
    }
}

// 判定事件
FeatureEvent Macro::DetermineEvent(u64 buttons) {
    if (m_Macros.empty()) return FeatureEvent::IDLE;
    if (m_IsPlaying) return HandlePlayingState(buttons);
    if (m_JustStopped) return HandleStopCooldown(buttons);
    return HandleNormalTrigger(buttons);
}

// 处理播放中状态
FeatureEvent Macro::HandlePlayingState(u64 buttons) {
    u64 currentCombo = m_Macros[m_CurrentMacroIndex].combo;
    bool isCurrentMacroPressed = (buttons & currentCombo) == currentCombo;
    // 检测到按下播放中的宏对应的快捷键，立即停止播放
    if (isCurrentMacroPressed && !m_HotkeyPressed) {
        m_HotkeyPressed = true;
        return FeatureEvent::FINISHING;
    }
    // 记录快捷键处于按下状态还是松开状态
    m_HotkeyPressed = isCurrentMacroPressed;
    if (m_Frames.empty()) return FeatureEvent::FINISHING;
    // 检查播放进度，如果是循环模式则重新开始
    if (CalculateTargetFrame() >= m_Frames.size()) {
        if (!m_RepeatMode) return FeatureEvent::FINISHING;
        m_PlaybackStartTick = armGetSystemTick();
        m_CurrentFrameIndex = 0;
    }
    return FeatureEvent::Macro_EXECUTING;
}

// 从FINISHING状态进入IDLE状态
FeatureEvent Macro::HandleStopCooldown(u64 buttons) {
    // 因为注入会污染数据，导致按键状态异常，所以等待一段时间
    u64 elapsedSinceStop = armTicksToNs(armGetSystemTick() - m_LastFinishTime);
    if (elapsedSinceStop < STOP_COOLDOWN_NS) return FeatureEvent::IDLE;
    // 检查对应的快捷键是否松开了
    u64 currentCombo = m_Macros[m_CurrentMacroIndex].combo;
    if ((buttons & currentCombo) != currentCombo) {
        m_JustStopped = false;
        m_HotkeyPressed = false;
        m_CurrentMacroIndex = -1;
    }
    return FeatureEvent::IDLE;
}

// 处理正常触发检测
FeatureEvent Macro::HandleNormalTrigger(u64 buttons) {
    // 检查是否有任何宏对应的快捷键被按下
    int triggered = CheckHotkeyTriggered(buttons);
    bool isAnyMacroPressed = (triggered != -1);
    // 检测到快捷键刚按下
    if (isAnyMacroPressed && !m_HotkeyPressed) {
        m_HotkeyPressTime = armGetSystemTick();
        m_CurrentMacroIndex = triggered;
    }
    // 检测到快捷键刚松开
    else if (!isAnyMacroPressed && m_HotkeyPressed) {
        u64 pressDuration = armTicksToNs(armGetSystemTick() - m_HotkeyPressTime);
        m_RepeatMode = (pressDuration >= LONG_PRESS_THRESHOLD_NS);
        m_HotkeyPressed = false;
        m_HotkeyPressTime = 0;
        return FeatureEvent::STARTING;
    }
    // 记录快捷键处于按下状态还是松开状态
    m_HotkeyPressed = isAnyMacroPressed;
    return FeatureEvent::IDLE;
}

int Macro::CheckHotkeyTriggered(u64 buttons) {
    for (size_t i = 0; i < m_Macros.size(); i++) {
        u64 combo = m_Macros[i].combo;
        if ((buttons & combo) == combo) return i;
    }
    return -1; 
}

// 计算当前应该播放第几帧
u32 Macro::CalculateTargetFrame() const {
    if (m_FrameRate == 0) return 0;
    u64 currentTick = armGetSystemTick();
    u64 elapsedTicks = currentTick - m_PlaybackStartTick;
    u64 ticksPerFrame = armGetSystemTickFreq() / m_FrameRate;
    return elapsedTicks / ticksPerFrame;
}

// 事件处理：启动宏
void Macro::MacroStarting() {
    m_IsPlaying = true;
    LoadMacroFile(m_Macros[m_CurrentMacroIndex].MacroFilePath);
    m_CurrentFrameIndex = 0;
    m_PlaybackStartTick = armGetSystemTick();
    m_HotkeyPressed = false;  // 重置状态，因为松开才触发
}

// 加载宏文件
void Macro::LoadMacroFile(const char* filePath) {
    m_Frames.clear();
    m_FrameRate = 0;
    FILE* file = fopen(filePath, "rb");
    if (!file) return;
    // 读取文件头
    MacroHeader header;
    if (fread(&header, sizeof(MacroHeader), 1, file) != 1) {
        fclose(file);
        return;
    }
    // 验证文件头
    if (header.magic[0] != 'K' || header.magic[1] != 'E' || 
        header.magic[2] != 'Y' || header.magic[3] != 'X') {
        fclose(file);
        return;
    }
    // 读取帧率和帧数
    m_FrameRate = header.frameRate;
    if (header.frameCount > 0) {
        m_Frames.resize(header.frameCount);
        if (fread(m_Frames.data(), sizeof(MacroFrame), header.frameCount, file) != header.frameCount) {
            // 读取失败，清空数据
            m_Frames.clear();
            m_FrameRate = 0;
        }
    }
    fclose(file);
}

// 事件处理：执行宏
void Macro::MacroExecuting(ProcessResult& result) {
    // 计算当前应该播放第几帧
    u32 targetFrameIndex = CalculateTargetFrame();
    if (targetFrameIndex >= m_Frames.size()) return;
    m_CurrentFrameIndex = targetFrameIndex;
    const MacroFrame& frame = m_Frames[m_CurrentFrameIndex];
    
    // 应用按键和摇杆数据
    result.OtherButtons = frame.keysHeld;  // 覆盖
    result.JoyconButtons = frame.keysHeld;  // 覆盖
    // 摇杆：宏数据不为0时覆盖物理输入，为0时保持物理输入
    if (frame.leftX != 0) result.analog_stick_l.x = frame.leftX;
    if (frame.leftY != 0) result.analog_stick_l.y = frame.leftY;
    if (frame.rightX != 0) result.analog_stick_r.x = frame.rightX;
    if (frame.rightY != 0) result.analog_stick_r.y = frame.rightY;
}

void Macro::MacroFinishing() {
    m_IsPlaying = false;
    m_CurrentFrameIndex = 0;
    m_PlaybackStartTick = 0;
    m_FrameRate = 0;
    m_Frames.clear();
    m_HotkeyPressTime = 0;
    m_RepeatMode = false;
    m_LastFinishTime = armGetSystemTick();
    m_JustStopped = true;  // 标记刚停止，需要等待冷静期
}

