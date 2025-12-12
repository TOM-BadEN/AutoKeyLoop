#include "game.hpp"
#include <minIni.h>
#include <cstring>

namespace {
    constexpr const char* WHITE_INI_PATH = "/config/KeyX/white.ini";
    
    int WhitelistCallback(const char* section, const char* key, const char* value, void* userData) {
        if (strcmp(section, "white") == 0) {
            u64 tid = strtoull(key, nullptr, 16);
            if (tid != 0) {
                auto* set = static_cast<std::unordered_set<u64>*>(userData);
                set->insert(tid);
            }
        }
        return 1;
    }
}

// 静态成员初始化
u64 GameMonitor::m_LastTid = 0;
std::unordered_set<u64> GameMonitor::s_whitelist;

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
    
    // 3. 过滤非游戏ID
    u8 type = (u8)(tid >> 56);
    if (type == 0x01) return tid;           // 游戏直接通过
    if (s_whitelist.count(tid)) return tid; // 白名单通过
    return 0;
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

// 加载白名单到内存
void GameMonitor::LoadWhitelist() {
    s_whitelist.clear();
    ini_browse(WhitelistCallback, &s_whitelist, WHITE_INI_PATH);
}
