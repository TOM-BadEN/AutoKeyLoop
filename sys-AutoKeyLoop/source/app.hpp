#pragma once
#include "autokeymanager.hpp"
#include "ipc.hpp"
#include <mutex>

// APP应用程序类
class App final {
private:

    AutoKeyManager* autokey_manager = nullptr;  // 自动按键管理器
    IPCServer* ipc_server = nullptr;            // IPC服务器
    std::mutex autokey_mutex;                   // 保护 autokey_manager 的互斥锁

    // 控制住循环是否结束的
    bool loop_error = false;
    
    // 上一次检测到的游戏 Title ID
    u64 last_game_tid = 0;
    
    // 初始化配置路径（确保目录存在）
    bool InitializeConfigPath();
    
    // 获取当前游戏 Title ID（仅游戏，非游戏返回0）
    u64 GetCurrentGameTitleId();
    
public:
    // 构造函数
    App();
    
    // 析构函数
    ~App();
    
    // 主循环函数
    void Loop();
    
    // 开启连发模块
    bool StartAutoKey();
    
    // 退出连发模块
    void StopAutoKey();
};