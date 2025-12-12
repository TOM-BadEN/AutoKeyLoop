#include "turbo.hpp"
#include <minIni.h>
#include <cstdlib>

// 构造函数
Turbo::Turbo(const char* config_path) {
    m_ButtonMask = 0;
    m_PressDurationNs = 100 * 1000000ULL;   // 默认100ms
    m_ReleaseDurationNs = 100 * 1000000ULL; // 默认100ms
    m_IsActive = false;
    m_IsPressed = false;
    m_TurboStartTime = 0;
    m_InitialPressTime = 0;
    // 自动加载配置
    LoadConfig(config_path);
}

// 加载配置
void Turbo::LoadConfig(const char* config_path) {
    // 读取按键掩码（连发白名单）
    char buttons_str[32];
    ini_gets("AUTOFIRE", "buttons", "0", buttons_str, sizeof(buttons_str), config_path);
    m_ButtonMask = strtoull(buttons_str, nullptr, 10);
    // 读取时间参数（毫秒转纳秒）
    int press_ms = ini_getl("AUTOFIRE", "presstime", 100, config_path);
    int release_ms = ini_getl("AUTOFIRE", "fireinterval", 100, config_path);
    m_PressDurationNs = (u64)press_ms * 1000000ULL;
    m_ReleaseDurationNs = (u64)release_ms * 1000000ULL;
    // 读取防止误触开关
    m_DelayStart = ini_getbool("AUTOFIRE", "delaystart", 1, config_path);

}

// 核心函数：处理输入
void Turbo::Process(ProcessResult& result) {
    // 分类按键
    u64 autokey_buttons = result.buttons & m_ButtonMask;
    u64 normal_buttons = result.buttons & ~m_ButtonMask;
    result.event = DetermineEvent(autokey_buttons);
    switch (result.event) {
        case FeatureEvent::IDLE:
        case FeatureEvent::PAUSED:
            return;
        case FeatureEvent::STARTING:
            TurboStarting();
            return;
        case FeatureEvent::Turbo_EXECUTING:
            TurboExecuting(autokey_buttons, normal_buttons, result);
            return;
        case FeatureEvent::FINISHING:
            TurboFinishing();
            result.OtherButtons = result.buttons;
            return;
        default:
            return;
    }
}

// 事件判定
FeatureEvent Turbo::DetermineEvent(u64 autokey_buttons) {
    bool has_autokey = (autokey_buttons != 0);
    bool turbo_active = m_IsActive;
    if (turbo_active && CheckRelease(autokey_buttons)) return FeatureEvent::FINISHING;
    else if (turbo_active) return FeatureEvent::Turbo_EXECUTING;
    else if (has_autokey) {
        if (m_InitialPressTime == 0) m_InitialPressTime = armGetSystemTick();
        u64 elapsed_ns = armTicksToNs(armGetSystemTick() - m_InitialPressTime);
        if (m_DelayStart && elapsed_ns < 200000000ULL) return FeatureEvent::IDLE;
        m_InitialPressTime = 0;
        return FeatureEvent::STARTING;
    }
    m_InitialPressTime = 0;
    return FeatureEvent::IDLE;
}

// 事件处理：启动连发
void Turbo::TurboStarting() {
    m_IsActive = true;
    m_IsPressed = true;
    m_TurboStartTime = armGetSystemTick();
}

// 事件处理：连发运行
void Turbo::TurboExecuting(u64 autokey_buttons, u64 normal_buttons, ProcessResult& result) {
    // 用绝对时间计算当前应该是按下还是松开
    u64 elapsed_ns = armTicksToNs(armGetSystemTick() - m_TurboStartTime);
    u64 cycle_ns = m_PressDurationNs + m_ReleaseDurationNs;
    u64 pos_in_cycle = elapsed_ns % cycle_ns;
    m_IsPressed = (pos_in_cycle < m_PressDurationNs);
    result.OtherButtons = m_IsPressed ? (normal_buttons | autokey_buttons) : normal_buttons;
}

// 事件处理：停止连发
void Turbo::TurboFinishing() {
    m_IsActive = false;
    m_IsPressed = false;
    m_TurboStartTime = 0;
    m_InitialPressTime = 0;
}

// 检测真松开（仅在按下周期检测，避免污染）
bool Turbo::CheckRelease(u64 autokey_buttons) {
    if (!m_IsPressed) return false;
    // 用绝对时间计算当前周期内的位置
    u64 elapsed_ns = armTicksToNs(armGetSystemTick() - m_TurboStartTime);
    u64 cycle_ns = m_PressDurationNs + m_ReleaseDurationNs;
    u64 pos_in_cycle = elapsed_ns % cycle_ns;
    // 按下周期开始后30ms内不检测松开
    if (pos_in_cycle < 30000000ULL) return false;
    if (autokey_buttons == 0) return true;
    return false;
}


