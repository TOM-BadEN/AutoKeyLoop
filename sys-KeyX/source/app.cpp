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
    // 直接尝试创建目录
    if (mkdir(CONFIG_DIR, 0777) == 0) return true;
    // 如果错误原因是已存在，也算true
    if (errno == EEXIST) return true;
    // 其他错误（如父目录不存在、权限不足等）
    return false;
}

// App类的实现
App::App() {
    // 允许写入日志
    // log_set_enabled(true);
    // 初始化配置路径
    if (!InitializeConfigPath()) return;
    // 初始化IPC服务
    if (!InitializeIPC()) return;
    // 初始化成功，允许主循环开始
    m_loop_error = false;
}

App::~App() {
    m_CurrentMappings.clear();
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
        LoadAutoFireConfig();
        if (m_GameInFocus) StartAutoKey();
    });
    
    // 设置关闭连发回调
    ipc_server->SetDisableAutoFireCallback([this]() {
        m_CurrentAutoEnable = false;
        StopAutoKey();
    });
    
    // 设置开启映射回调
    ipc_server->SetEnableMappingCallback([this]() {
        m_CurrentAutoRemapEnable = true;
        LoadButtonMappingConfig();
        if (m_GameInFocus) ButtonRemapper::SetMapping(m_CurrentMappings);
    });
    
    // 设置关闭映射回调
    ipc_server->SetDisableMappingCallback([this]() {
        m_CurrentAutoRemapEnable = false;
        ButtonRemapper::RestoreMapping();
    });
    
    // 设置重载全部配置
    ipc_server->SetReloadBasicCallback([this]() {
        LoadGameConfig(m_CurrentTid);
        if (m_GameInFocus && m_CurrentAutoRemapEnable) ButtonRemapper::SetMapping(m_CurrentMappings);
        UpdateAutoKeyConfig();
    });
    
    // 设置重载连发配置回调
    ipc_server->SetReloadAutoFireCallback([this]() {
        LoadAutoFireConfig();
        UpdateAutoKeyConfig();
    });
    
    // 设置重载映射配置回调
    ipc_server->SetReloadMappingCallback([this]() {
        LoadButtonMappingConfig();
        if (m_GameInFocus && m_CurrentAutoRemapEnable) ButtonRemapper::SetMapping(m_CurrentMappings);
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
                if (m_CurrentAutoEnable || m_CurrentAutoRemapEnable) svcSleepThread(100000000ULL);
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
    m_GameInFocus = true;
    m_CurrentTid = tid;
    LoadGameConfig(tid);
    if (m_CurrentAutoEnable) StartAutoKey();
    if (m_CurrentAutoRemapEnable) ButtonRemapper::SetMapping(m_CurrentMappings);
    CreateNotification(true);
}

// 处理游戏运行事件
void App::OnGameRunning(u64 tid) {
    // 获取游戏焦点状态
    FocusState focus = FocusMonitor::GetState(tid);
    switch (focus) {
        case FocusState::InFocus:
            m_GameInFocus = true;
            if (m_CurrentAutoEnable && autokey_manager != nullptr) ResumeAutoKey();
            else if (m_CurrentAutoEnable && autokey_manager == nullptr) StartAutoKey();
            if (m_CurrentAutoRemapEnable) ButtonRemapper::SetMapping(m_CurrentMappings);
            break;
        case FocusState::OutOfFocus:
            m_GameInFocus = false;
            if (m_CurrentAutoEnable) PauseAutoKey();
            if (m_CurrentAutoRemapEnable) ButtonRemapper::RestoreMapping();
            break;
        default:
            break;
    }
}

// 处理游戏退出事件
void App::OnGameExited() {
    m_GameInFocus = false;
    if (m_CurrentAutoEnable) StopAutoKey();
    if (m_CurrentAutoRemapEnable) ButtonRemapper::RestoreMapping();
    m_CurrentTid = 0;
    CreateNotification(false);
}

// 加载游戏配置（读取并缓存配置参数）
void App::LoadGameConfig(u64 tid) {
    LoadBasicConfig(tid);
    LoadAutoFireConfig();
    LoadButtonMappingConfig();
}

// 加载基础配置（确定配置路径）
void App::LoadBasicConfig(u64 tid) {
    m_notifEnabled = ini_getbool("NOTIFICATION", "notif", false, CONFIG_PATH);
    snprintf(m_GameConfigPath, sizeof(m_GameConfigPath), "/config/KeyX/GameConfig/%016lX.ini", tid);
    // 此处用来设定是否使用全局配置，还是独立配置
    // 功能的开关，是m_SwitchConfigPath
    // 功能的详细参数，是m_ConfigPath
    m_CurrentGlobConfig = ini_getbool("AUTOFIRE", "globconfig", 1, m_GameConfigPath);
    m_ConfigPath = m_CurrentGlobConfig ? CONFIG_PATH : m_GameConfigPath;
    m_SwitchConfigPath = FileExists(m_GameConfigPath) ? m_GameConfigPath : CONFIG_PATH;  
    m_CurrentAutoEnable = ini_getbool("AUTOFIRE", "autoenable", 0, m_SwitchConfigPath);
    m_CurrentAutoRemapEnable = ini_getbool("MAPPING", "autoenable", 0, m_SwitchConfigPath);
}

// 加载连发配置
void App::LoadAutoFireConfig() {
    char buttons_str[32];
    ini_gets("AUTOFIRE", "buttons", "0", buttons_str, sizeof(buttons_str), m_ConfigPath);
    m_CurrentButtons = strtoull(buttons_str, nullptr, 10);
    m_CurrentPressTime = ini_getl("AUTOFIRE", "presstime", 100, m_ConfigPath);
    m_CurrentFireInterval = ini_getl("AUTOFIRE", "fireinterval", 100, m_ConfigPath);
}

// 加载映射配置
void App::LoadButtonMappingConfig() {
    LoadButtonMappings(m_ConfigPath);
}

// 加载按键映射配置
void App::LoadButtonMappings(const char* config_path) {
    // 清空旧数据
    m_CurrentMappings.clear();
    // 定义所有按键名称（编译时常量）
    constexpr const char* button_names[] = {
        "A", "B", "X", "Y",
        "Up", "Down", "Left", "Right",
        "L", "R", "ZL", "ZR",
        "StickL", "StickR", "Start", "Select"
    };
    // 逐个读取映射配置
    for (int i = 0; i < 16; i++) {
        ButtonMapping mapping;
        // 设置源按键名称（指向字符串字面量）
        mapping.source = button_names[i];
        // 从 INI 读取目标按键（默认值为源按键名称，表示未映射）
        ini_gets("MAPPING", button_names[i], button_names[i], mapping.target, sizeof(mapping.target), config_path);
        // 只添加有映射变化的按键（跳过 A=A 这种未映射的）
        m_CurrentMappings.push_back(mapping);
    }
}

// 开启连发模块
bool App::StartAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    // 如果已经创建，则不重复创建
    if (autokey_manager != nullptr) return true;
    // 创建并初始化连发模块
    autokey_manager = std::make_unique<AutoKeyManager>(
        m_CurrentButtons,
        m_CurrentPressTime,
        m_CurrentFireInterval
    );
    return true;
}

// 退出连发模块
void App::StopAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_manager != nullptr) autokey_manager.reset();
}

// 暂停连发
void App::PauseAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_manager != nullptr) autokey_manager->Pause();
}

// 恢复连发
void App::ResumeAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_manager != nullptr) autokey_manager->Resume();
}

// 更新连发配置
void App::UpdateAutoKeyConfig() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    if (autokey_manager != nullptr) {
        autokey_manager->UpdateConfig(m_CurrentButtons, m_CurrentPressTime, m_CurrentFireInterval);
    }
}

void App::ShowNotification(const char* message) {
    if (m_notifEnabled) createNotification(message, 2, INFO, RIGHT);
}

void App::CreateNotification(bool Enable) {
    if (!m_notifEnabled) return;
    if (!m_CurrentAutoEnable && !m_CurrentAutoRemapEnable) return;
    const char* message = nullptr;
    if (m_CurrentAutoEnable && m_CurrentAutoRemapEnable) {
        message = Enable ? g_NOTIF_ALL_ON : g_NOTIF_ALL_OFF;
    } else if (m_CurrentAutoEnable) {
        message = Enable ? g_NOTIF_AUTOFIRE_ON : g_NOTIF_AUTOFIRE_OFF;
    } else if (m_CurrentAutoRemapEnable) {
        message = Enable ? g_NOTIF_MAPPING_ON : g_NOTIF_MAPPING_OFF;
    } 
    if (message) createNotification(message, 2, INFO, RIGHT);
}

