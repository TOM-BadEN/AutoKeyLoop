#pragma once
#include "autokeymanager.hpp"
#include "ipc.hpp"
#include "buttonremapper.hpp"
#include "focusmonitor.hpp"
#include "gamemonitor.hpp"
#include <mutex>
#include <memory>
#include <vector>

// APP应用程序类
class App final {
private:

    std::unique_ptr<AutoKeyManager> autokey_manager;  // 自动按键管理器
    std::unique_ptr<IPCServer> ipc_server;            // IPC服务器
    std::mutex autokey_mutex;                         // 保护 autokey_manager 的互斥锁

    // 控制住循环是否结束的true代表停止循环
    bool m_loop_error = true;

    // 通知功能
    bool m_notifEnabled = false;             // 通知开关

    // 当前使用的配置文件路径
    char m_GameConfigPath[64];               // 游戏配置文件路径
    const char* m_ConfigPath;                // 其他配置路径（根据globconfig决定）
    const char* m_SwitchConfigPath;          // 开关配置路径（优先游戏配置）

    // 当前游戏是否在焦点中
    bool m_GameInFocus = false;

    // 连发功能相关配置
    u64 m_CurrentTid = 0;                    // 当前游戏 TID
    u64 m_CurrentButtons = 0;                // 白名单按键
    int m_CurrentPressTime = 60;             // 按下时长（毫秒）
    int m_CurrentFireInterval = 160;         // 松开时长（毫秒）
    bool m_CurrentAutoEnable = false;        // 是否自动启动
    bool m_CurrentGlobConfig = true;         // 是否使用全局配置

    // 按键映射功能相关配置
    std::vector<ButtonMapping> m_CurrentMappings;  // 按键映射配置
    bool m_CurrentAutoRemapEnable = false;         // 是否自动启动
    
    // 初始化配置路径（确保目录存在）
    bool InitializeConfigPath();
    
    // 初始化IPC服务
    bool InitializeIPC();
    
    // 检查文件是否存在
    bool FileExists(const char* path);
    
    // 加载游戏配置（读取并缓存配置参数）
    void LoadGameConfig(u64 tid);
    
    // 加载基础配置（确定配置路径）
    void LoadBasicConfig(u64 tid);
    
    // 加载连发配置
    void LoadAutoFireConfig();
    
    // 加载映射配置
    void LoadButtonMappingConfig();
    
    // 加载按键映射（内部调用）
    void LoadButtonMappings(const char* config_path);
    
    // 获取当前游戏 Title ID（仅游戏，非游戏返回0）
    u64 GetCurrentGameTitleId();
    
    // 游戏事件处理函数
    void OnGameLaunched(u64 tid);
    void OnGameRunning(u64 tid);
    void OnGameExited();

    // 触发弹窗
    void ShowNotification(const char* message);
    
    // 创建通知
    void CreateNotification(bool Enable);
    
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
    
    // 暂停连发（保持模块运行）
    void PauseAutoKey();
    
    // 恢复连发（取消暂停）
    void ResumeAutoKey();
    
    // 更新连发配置（线程安全）
    void UpdateAutoKeyConfig();
    
};