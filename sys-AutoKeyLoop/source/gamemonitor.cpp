#include "gamemonitor.hpp"

// 静态成员初始化
u64 GameMonitor::m_LastTid = 0;

// 获取当前游戏 Title ID（仅游戏，非游戏返回0）
u64 GameMonitor::GetCurrentGameTitleId() {
    u64 pid = 0, tid = 0;
    
    // 1. 获取当前应用的进程ID
    if (R_FAILED(pmdmntGetApplicationProcessId(&pid))) {
        return 0;
    }

    // 2. 根据进程ID获取程序ID (Title ID)
    if (R_FAILED(pmdmntGetProgramId(&tid, pid))) {
        return 0;
    }
    
    // 3. 过滤非游戏ID - 检查Title ID是否为游戏格式（高32位以0x01开头）
    u32 high_part = (u32)(tid >> 32);
    if ((high_part & 0xFF000000) != 0x01000000) {
        return 0;  // 不是游戏ID，返回0
    }
    
    return tid;
}

// 检查游戏状态（返回事件+TID）
GameStateResult GameMonitor::GetState() {
    u64 tid = GetCurrentGameTitleId();
    
    // 游戏退出（tid → 0）
    if (tid == 0 && m_LastTid != 0) {
        m_LastTid = 0;
        return {GameEvent::Exited, 0};
    }
    
    // 游戏启动（0 → tid）
    if (tid != 0 && m_LastTid == 0) {
        m_LastTid = tid;
        return {GameEvent::Launched, tid};
    }
    
    // 无游戏（一直是 0）
    if (tid == 0) {
        return {GameEvent::Idle, 0};
    }
    
    // 游戏持续运行（tid 不变）
    return {GameEvent::Running, tid};
}

