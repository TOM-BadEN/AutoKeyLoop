#include <switch.h>
#include "app.hpp"
#include <errno.h>
#include <sys/stat.h>
#include <minIni.h>
#include <cstdlib>

#define CONFIG_DIR "/config/AutoKeyLoop"
#define CONFIG_PATH "/config/AutoKeyLoop/config.ini"


// 检查文件是否存在
bool App::FileExists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

// 初始化配置路径（确保配置目录存在）
bool App::InitializeConfigPath() {

    // 直接尝试创建目录
    if (mkdir(CONFIG_DIR, 0777) == 0) {
        // 创建成功
        return true;
    }
    
    // 如果错误原因是已存在，也算true
    if (errno == EEXIST) {
        return true;
    }
    
    // 其他错误（如父目录不存在、权限不足等）
    return false;
}

// App类的实现
App::App() {

    // 初始化配置路径
    if (!InitializeConfigPath()) {
        // 失败则退出程序
        m_loop_error = true;
        return;
    }
    
    // 创建并启动IPC服务
    ipc_server = new IPCServer();
    
    if (!ipc_server->Start("keyLoop")) {
        delete ipc_server;
        ipc_server = nullptr;
        m_loop_error = true;
        return;
    }

    // 设置IPC退出回调：当收到退出命令时，标记主循环退出
    ipc_server->SetExitCallback([this]() {
        m_loop_error = true;
    });
    
    // 设置IPC开启连发回调
    ipc_server->SetEnableCallback([this]() {
        LoadGameConfig(m_CurrentTid);
        StartAutoKey();
    });
    
    // 设置IPC关闭连发回调
    ipc_server->SetDisableCallback([this]() {
        StopAutoKey();
    });
    
    // 设置IPC重载配置回调
    ipc_server->SetReloadConfigCallback([this]() {
        // 重新加载配置到缓存变量
        LoadGameConfig(m_CurrentTid);
        
        // 如果连发模块正在运行，动态更新其配置
        std::lock_guard<std::mutex> lock(autokey_mutex);
        if (autokey_manager != nullptr) {
            autokey_manager->UpdateConfig(
                m_CurrentButtons,
                m_CurrentPressTime,
                m_CurrentFireInterval
            );
        }
    });
    
}

App::~App() {

    // 清理IPC服务器
    if (ipc_server) {
        ipc_server->Stop();
        delete ipc_server;
        ipc_server = nullptr;
    }
    
    // 清理AutoKeyManager
    if (autokey_manager) {
        delete autokey_manager;
        autokey_manager = nullptr;
    }
}

void App::Loop() {

    int check_counter = 0;
    
    while (!m_loop_error) {
        check_counter++;
        // 每次只睡眠100ms，快速检查退出标志
        svcSleepThread(100000000ULL);
        
        // 提前跳过，减少嵌套
        if (check_counter < 10) continue;
        
        // 执行游戏TID检测
        check_counter = 0;  // 重置计数器
        u64 current_tid = GetCurrentGameTitleId();
        
        // 程序为改变，跳过
        if (current_tid == m_last_game_tid) continue;
        
        // 检测到游戏状态变化
        m_last_game_tid = current_tid;
        
        if (current_tid != 0) {
            // 加载新的连发配置
            LoadGameConfig(current_tid);
            if (m_CurrentAutoEnable) {
                StartAutoKey();
            }
        } else {
            // 游戏退出
            StopAutoKey();
        }
    }
}

// 加载游戏配置（读取并缓存配置参数）
void App::LoadGameConfig(u64 tid) {
    // 构建游戏配置文件路径 (固定长度: 55字符)
    char game_config_path[64];
    snprintf(game_config_path, sizeof(game_config_path), 
             "/config/AutoKeyLoop/GameConfig/%016lX.ini", tid);
    
    const char* config_to_use = CONFIG_PATH;  // 默认使用全局配置
    
    // 检查游戏配置文件是否存在
    if (FileExists(game_config_path)) {
        // 游戏配置文件存在，读取 globconfig
        m_CurrentGlobConfig = ini_getbool("AUTOFIRE", "globconfig", 1, game_config_path);
        m_CurrentAutoEnable = ini_getbool("AUTOFIRE", "autoenable", 0, game_config_path);
        // 如果游戏配置文件中要求不使用全局配置
        if (!m_CurrentGlobConfig) {
            // 使用游戏独立配置
            config_to_use = game_config_path;
        }
    } else {
        // 游戏配置文件不存在，使用全局配置
        m_CurrentGlobConfig = true;
        m_CurrentAutoEnable = ini_getbool("AUTOFIRE", "autoenable", 0, CONFIG_PATH);
    }
    
    // 读取按键配置（字符串格式）
    char buttons_str[32];
    ini_gets("AUTOFIRE", "buttons", "0", buttons_str, sizeof(buttons_str), config_to_use);
    m_CurrentButtons = strtoull(buttons_str, nullptr, 10);
    
    // 读取时间配置
    m_CurrentPressTime = ini_getl("AUTOFIRE", "presstime", 100, config_to_use);
    m_CurrentFireInterval = ini_getl("AUTOFIRE", "fireinterval", 100, config_to_use);
    
    // 更新当前 TID
    m_CurrentTid = tid;
}

// 获取当前游戏 Title ID（仅游戏，非游戏返回0）
u64 App::GetCurrentGameTitleId() {
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

// 开启连发模块
bool App::StartAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    
    // 如果已经创建，则不重复创建
    if (autokey_manager != nullptr) {
        return true;
    }
    
    // 创建并初始化连发模块
    autokey_manager = new AutoKeyManager(
        m_CurrentButtons,
        m_CurrentPressTime,
        m_CurrentFireInterval
    );
    
    if (autokey_manager == nullptr) {
        return false;
    }
    
    return true;
}

// 退出连发模块
void App::StopAutoKey() {
    std::lock_guard<std::mutex> lock(autokey_mutex);
    
    // 如果没有创建，则无需清理
    if (autokey_manager == nullptr) {
        return;
    }
    
    // 清理连发模块
    delete autokey_manager;
    autokey_manager = nullptr;
}