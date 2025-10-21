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
    bool m_loop_error = false;
    
    // 上一次检测到的游戏 Title ID
    u64 m_last_game_tid = 0;
    
    // 当前游戏的配置缓存
    bool m_notifEnabled = false;             // 通知开关
    u64 m_CurrentTid = 0;                    // 当前游戏 TID
    u64 m_CurrentButtons = 0;                // 白名单按键
    int m_CurrentPressTime = 60;             // 按下时长（毫秒）
    int m_CurrentFireInterval = 160;         // 松开时长（毫秒）
    bool m_CurrentAutoEnable = false;        // 是否自动启动
    bool m_CurrentGlobConfig = true;         // 是否使用全局配置

    // IPC工作标志
    bool m_IPCWorking = false;
    
    // 初始化配置路径（确保目录存在）
    bool InitializeConfigPath();
    
    // 检查文件是否存在
    bool FileExists(const char* path);
    
    // 加载游戏配置（读取并缓存配置参数）
    void LoadGameConfig(u64 tid);
    
    // 获取当前游戏 Title ID（仅游戏，非游戏返回0）
    u64 GetCurrentGameTitleId();

    // 触发弹窗
    void ShowNotificationAndResetIPCFlag(const char* message);
    
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