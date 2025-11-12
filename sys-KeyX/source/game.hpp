#pragma once
#include <switch.h>

// 游戏事件类型
enum class GameEvent : uint8_t {
    Idle = 0,          // 无游戏运行
    Running = 1,       // 游戏持续运行
    Launched = 2,      // 游戏启动
    Exited = 3         // 游戏退出
};

// 游戏状态结果
struct GameStateResult {
    GameEvent event;
    u64 tid;
};

// 游戏监控类（静态类）
class GameMonitor {
public:
    // 检查游戏状态（返回事件+TID）
    static GameStateResult GetState();
    
private:
    // 获取当前游戏 Title ID（仅游戏，非游戏返回0）
    static u64 GetCurrentGameTitleId();
    
    // 上次检测到的游戏 TID
    static u64 m_LastTid;
};

