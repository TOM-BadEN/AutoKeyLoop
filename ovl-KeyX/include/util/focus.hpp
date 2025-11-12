#pragma once
#include <switch.h>

// 焦点状态枚举
enum class FocusState : uint8_t {
    InFocus = 1,      // 游戏前台
    OutOfFocus = 2,   // 游戏后台
};

// 焦点监控器（全静态）
class FocusMonitor {
public:
    // 获取焦点状态（自动更新）
    // tid: 当前游戏 TID（必须 > 0）
    // 返回: InFocus=游戏前台, OutOfFocus=游戏后台
    static FocusState GetState(u64 tid);
    
private:
    static void ResetForNewGame();  // 重置状态（游戏切换时）
    
    // 辅助函数：从事件数组中查找指定tid的最后一个焦点状态
    static FocusState FindLastFocusState(const PdmAppletEvent* events, s32 total_out, u64 tid);
    
    static u64 m_LastTid;              // 上次的游戏 TID（用于检测游戏切换）
    static s32 m_LastCheckedIndex;     // pdmqry 上次检查的索引
    static FocusState m_CurrentState;  // 当前焦点状态
};

