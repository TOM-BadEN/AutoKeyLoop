#pragma once
#include <switch.h>

// 焦点状态枚举
enum class FocusState : uint8_t {
    Unknown = 0,      // 未变化
    InFocus = 1,      // 变为前台
    OutOfFocus = 2,   // 变为后台
};

// 焦点监控器（全静态）
class FocusMonitor {
public:
    // 获取焦点状态（自动更新）
    // tid: 当前游戏 TID（必须 > 0）
    // 返回: Unknown=未变化, InFocus=刚进入前台, OutOfFocus=刚进入后台
    static FocusState GetState(u64 tid);
    
private:
    static void ResetForNewGame();  // 重置状态（游戏切换时）
    
    static u64 m_LastTid;              // 上次的游戏 TID（用于检测游戏切换）
    static s32 m_LastCheckedIndex;     // pdmqry 上次检查的索引
    static FocusState m_CurrentState;  // 当前焦点状态
};

