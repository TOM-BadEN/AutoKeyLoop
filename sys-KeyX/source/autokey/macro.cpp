#include "macro.hpp"
#include "minIni.h"
#include <cstdio>

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
        case FeatureEvent::EXECUTING:
            MacroExecuting(result);
            return;
        case FeatureEvent::FINISHING:
            MacroFinishing();
            return;
    }
}

// 判定事件
FeatureEvent Macro::DetermineEvent(u64 buttons) {
    if (m_Macros.empty()) return FeatureEvent::IDLE;
    if (m_IsPlaying) {
        int triggered = CheckHotkeyTriggered(buttons);
        bool currentPressed = (triggered == m_CurrentMacroIndex);
        if (currentPressed && !m_LastHotkeyPressed) {
            m_LastHotkeyPressed = currentPressed;
            return FeatureEvent::FINISHING;
        }
        m_LastHotkeyPressed = currentPressed;
        if (m_Frames.empty()) return FeatureEvent::FINISHING;
        u32 targetFrameIndex = CalculateTargetFrame();
        if (targetFrameIndex >= m_Frames.size()) return FeatureEvent::FINISHING;
        return FeatureEvent::EXECUTING;
    }
    if (CheckHotkeyTriggered(buttons) != -1) return FeatureEvent::STARTING;
    return FeatureEvent::IDLE;
}

int Macro::CheckHotkeyTriggered(u64 buttons) {
    for (size_t i = 0; i < m_Macros.size(); i++) {
        u64 combo = m_Macros[i].combo;
        if ((buttons & combo) == combo) return m_CurrentMacroIndex = i;
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
    m_LastHotkeyPressed = true;
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
    result.OtherButtons = frame.keysHeld;
    result.JoyconButtons = frame.keysHeld;
    result.analog_stick_l.x = frame.leftX;
    result.analog_stick_l.y = frame.leftY;
    result.analog_stick_r.x = frame.rightX;
    result.analog_stick_r.y = frame.rightY;
}

void Macro::MacroFinishing() {
    m_IsPlaying = false;
    m_CurrentMacroIndex = -1;
    m_CurrentFrameIndex = 0;
    m_PlaybackStartTick = 0;
    m_FrameRate = 0;
    m_Frames.clear();
    m_LastHotkeyPressed = false;
}

