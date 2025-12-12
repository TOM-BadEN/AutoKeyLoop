#include <switch.h>
#include "app.hpp"
#include <errno.h>
#include <sys/stat.h>
#include <minIni.h>
#include <cstdlib>
#include "libnotification.h"

#define CONFIG_DIR "/config/KeyX"
#define CONFIG_PATH "/config/KeyX/config.ini"

extern const char* g_NOTIF_AUTOFIRE_ON;
extern const char* g_NOTIF_AUTOFIRE_OFF;
extern const char* g_NOTIF_MAPPING_ON;
extern const char* g_NOTIF_MAPPING_OFF;
extern const char* g_NOTIF_ALL_ON;
extern const char* g_NOTIF_ALL_OFF;


// 检查文件是否存在
bool App::FileExists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

// 初始化配置路径（确保配置目录存在）
bool App::InitializeConfigPath() {
    if (mkdir(CONFIG_DIR, 0777) == 0) return true;
    if (errno == EEXIST) return true;
    return false;
}

// App类的实现
App::App() {
    if (!InitializeConfigPath()) return;
    if (!InitializeIPC()) return;
    m_loop_error = false;
}

App::~App() {
}

// 初始化IPC服务
bool App::InitializeIPC() {
    // 创建IPC服务
    ipc_server = std::make_unique<IPCServer>();

    // 设置退出回调
    ipc_server->SetExitCallback([this]() {
        ButtonRemapper::RestoreMapping();
        m_loop_error = true;
    });
    
    // 设置开启连发回调
    ipc_server->SetEnableAutoFireCallback([this]() {
        m_CurrentAutoEnable = true;
        if (m_GameInFocus) {
            if (autokey_loop) UpdateTurboConfig();
            else StartAutoKey();
        }
    });
    
    // 设置关闭连发回调
    ipc_server->SetDisableAutoFireCallback([this]() {
        m_CurrentAutoEnable = false;
        if (!m_CurrentAutoMacroEnable) StopAutoKey();
        else UpdateTurboConfig();
    });
    
    // 设置开启宏回调
    ipc_server->SetEnableMacroCallback([this]() {
        m_CurrentAutoMacroEnable = true;
        if (m_GameInFocus) {
            if (autokey_loop) UpdateMacroConfig();
            else StartAutoKey();
        }
    });
    
    // 设置关闭宏回调
    ipc_server->SetDisableMacroCallback([this]() {
        m_CurrentAutoMacroEnable = false;
        if (!m_CurrentAutoEnable) StopAutoKey();
        else UpdateMacroConfig();
    });
    
    // 设置开启映射回调
    ipc_server->SetEnableMappingCallback([this]() {
        m_CurrentAutoRemapEnable = true;
        if (m_GameInFocus) ButtonRemapper::SetMapping(m_ConfigPath);
        UpdateButtonMappingConfig();
    });
    
    // 设置关闭映射回调
    ipc_server->SetDisableMappingCallback([this]() {
        m_CurrentAutoRemapEnable = false;
        ButtonRemapper::RestoreMapping();
    });
    
    // 设置重载全部配置
    ipc_server->SetReloadBasicCallback([this]() {
        LoadGameConfig(m_CurrentTid);
        if (m_GameInFocus && m_CurrentAutoRemapEnable) ButtonRemapper::SetMapping(m_ConfigPath);
        UpdateTurboConfig();
        UpdateMacroConfig();
        UpdateButtonMappingConfig();
    });
    
    // 设置重载连发配置回调
    ipc_server->SetReloadAutoFireCallback([this]() {
        UpdateTurboConfig();
    });
    
    // 设置重载宏配置回调
    ipc_server->SetReloadMacroCallback([this]() {
        UpdateMacroConfig();
    });
    
    // 设置重载映射配置回调
    ipc_server->SetReloadMappingCallback([this]() {
        if (m_GameInFocus && m_CurrentAutoRemapEnable) ButtonRemapper::SetMapping(m_ConfigPath);
        UpdateButtonMappingConfig();
    });

    // 重载白名单
    ipc_server->SetReloadWhitelistCallback([this]() {
        GameMonitor::LoadWhitelist();
    });

    // 启动服务
    if (!ipc_server->Start("keyLoop")) {
        ipc_server.reset();
        return false;
    }

    return true;
}


void App::Loop() {
    while (!m_loop_error) {
        GameStateResult game = GameMonitor::GetState();
        switch (game.event) {
            case GameEvent::Idle:
                // 无游戏运行
                for (int i = 0; i < 10 && !m_loop_error; ++i) svcSleepThread(100000000ULL);
                continue;
            case GameEvent::Running:
                OnGameRunning(game.tid);
                if (autokey_loop || m_CurrentAutoRemapEnable) svcSleepThread(100000000ULL);
                else for (int i = 0; i < 5 && !m_loop_error; ++i) svcSleepThread(100000000ULL);
                continue;
            case GameEvent::Launched:
                OnGameLaunched(game.tid);
                break;
            case GameEvent::Exited:
                OnGameExited();
                break;
            default:
                break;
        }
        svcSleepThread(100000000ULL);  // 100ms
    }
}

// 处理游戏启动事件
void App::OnGameLaunched(u64 tid) {
    m_FirstLaunch = true;
    m_GameInFocus = true;
    m_CurrentTid = tid;
    LoadGameConfig(tid);
    if (m_CurrentAutoEnable || m_CurrentAutoMacroEnable) StartAutoKey();
    if (m_CurrentAutoRemapEnable) ButtonRemapper::SetMapping(m_ConfigPath);
    CreateNotification(true);
}

// 处理游戏运行事件
void App::OnGameRunning(u64 tid) {
    // 获取游戏焦点状态
    FocusState focus = FocusMonitor::GetState(tid);
    switch (focus) {
        case FocusState::InFocus:
            m_GameInFocus = true;
            if ((m_CurrentAutoEnable || m_CurrentAutoMacroEnable) && autokey_loop) ResumeAutoKey();
            else if ((m_CurrentAutoEnable || m_CurrentAutoMacroEnable) && !autokey_loop) StartAutoKey();
            else if (!m_CurrentAutoEnable && !m_CurrentAutoMacroEnable && autokey_loop) StopAutoKey();
            if (m_CurrentAutoRemapEnable) ButtonRemapper::SetMapping(m_ConfigPath);
            break;
        case FocusState::OutOfFocus:
            m_GameInFocus = false;
            if (autokey_loop) PauseAutoKey();
            if (m_CurrentAutoRemapEnable) ButtonRemapper::RestoreMapping();
            break;
        default:
            break;
    }
}

// 处理游戏退出事件
void App::OnGameExited() {
    m_GameInFocus = false;
    if (autokey_loop) StopAutoKey();
    m_CurrentTid = 0;
    CreateNotification(false);
}

// 加载游戏配置（读取并缓存配置参数）
void App::LoadGameConfig(u64 tid) {
    LoadBasicConfig(tid);
}

// 加载基础配置（确定配置路径）
void App::LoadBasicConfig(u64 tid) {
    m_notifEnabled = ini_getbool("NOTIFICATION", "notif", false, CONFIG_PATH);
    snprintf(m_GameConfigPath, sizeof(m_GameConfigPath), "/config/KeyX/GameConfig/%016lX.ini", tid);
    m_CurrentGlobConfig = ini_getbool("AUTOFIRE", "globconfig", 1, m_GameConfigPath);
    if (m_FirstLaunch && m_CurrentGlobConfig) {
        bool defaultAutoEnable = ini_getbool("AUTOFIRE", "defaultautoenable", 0, CONFIG_PATH);
        bool defaultRemapEnable = ini_getbool("MAPPING", "defaultautoenable", 0, CONFIG_PATH);
        ini_putl("AUTOFIRE", "autoenable", defaultAutoEnable, CONFIG_PATH);
        ini_putl("MAPPING", "autoenable", defaultRemapEnable, CONFIG_PATH);
    }
    m_FirstLaunch = false;
    // 此处用来设定是否使用全局配置，还是独立配置
    // 功能的详细参数，是m_ConfigPath
    // 功能的开关，是SwitchConfigPath
    // 宏的开关，是 m_GameConfigPath
    m_ConfigPath = m_CurrentGlobConfig ? CONFIG_PATH : m_GameConfigPath;
    const char* switchConfigPath = m_CurrentGlobConfig ? CONFIG_PATH : m_GameConfigPath;
    m_CurrentAutoEnable = ini_getbool("AUTOFIRE", "autoenable", 0, switchConfigPath);
    m_CurrentAutoRemapEnable = ini_getbool("MAPPING", "autoenable", 0, switchConfigPath);
    m_CurrentAutoMacroEnable = ini_getbool("MACRO", "autoenable", 0, m_GameConfigPath);  // 宏只读取独立配置
}

// 开启按键模块
bool App::StartAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    // 如果已经创建，则不重复创建
    if (autokey_loop) return true;
    autokey_loop = std::make_unique<AutoKeyLoop>(m_ConfigPath, m_GameConfigPath, m_CurrentAutoEnable, m_CurrentAutoMacroEnable);
    return true;
}

// 退出按键模块
void App::StopAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_loop) autokey_loop.reset();
}

// 暂停连发
void App::PauseAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_loop) autokey_loop->Pause();
}

// 恢复连发
void App::ResumeAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_loop) autokey_loop->Resume();
}

// 更新连发配置
void App::UpdateTurboConfig() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_loop) {
        autokey_loop->UpdateTurboFeature(m_CurrentAutoEnable, m_ConfigPath);
    }
}

// 更新宏配置
void App::UpdateMacroConfig() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_loop) {
        autokey_loop->UpdateMacroFeature(m_CurrentAutoMacroEnable, m_GameConfigPath);
    }
}

// 更新按键映射配置
void App::UpdateButtonMappingConfig() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_loop) {
        autokey_loop->UpdateButtonMappings(m_ConfigPath);
    }
}

void App::ShowNotification(const char* message) {
    if (m_notifEnabled) createNotification(message, 2, INFO, RIGHT);
}

void App::CreateNotification(bool Enable) {
    if (!m_notifEnabled) return;
    if (!m_CurrentAutoEnable && !m_CurrentAutoRemapEnable && !m_CurrentAutoMacroEnable) return;
    const char* message = nullptr;
    if (m_CurrentAutoEnable && m_CurrentAutoRemapEnable && m_CurrentAutoMacroEnable) {
        message = Enable ? g_NOTIF_ALL_ON : g_NOTIF_ALL_OFF;
    } else if (m_CurrentAutoEnable || m_CurrentAutoMacroEnable) {
        message = Enable ? g_NOTIF_AUTOFIRE_ON : g_NOTIF_AUTOFIRE_OFF;
    } else if (m_CurrentAutoRemapEnable) {
        message = Enable ? g_NOTIF_MAPPING_ON : g_NOTIF_MAPPING_OFF;
    } 
    if (message) createNotification(message, 2, INFO, RIGHT);
}

