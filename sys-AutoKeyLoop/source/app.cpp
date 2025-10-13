#include <switch.h>
#include "app.hpp"
#include "../log/log.h"
#include <errno.h>
#include <sys/stat.h>
#include <minIni.h>

#define CONFIG_DIR "/config/AutoKeyLoop"
#define CONFIG_PATH "/config/AutoKeyLoop/config.ini"


// 初始化配置路径（确保配置目录存在）
bool App::InitializeConfigPath() {

    // 直接尝试创建目录
    if (mkdir(CONFIG_DIR, 0777) == 0) {
        // 创建成功
        log_info("创建配置文件目录: %s", CONFIG_DIR);
        return true;
    }
    
    // 如果错误原因是已存在，也算true
    if (errno == EEXIST) {
        log_info("配置目录已存在: %s", CONFIG_DIR);
        return true;
    }
    
    // 其他错误（如父目录不存在、权限不足等）
    log_error("配置目录创建失败: %s (errno: %d)", CONFIG_DIR, errno);
    return false;
}

// App类的实现
App::App() {

    // 读取日志配置并设置日志开关
    bool log_enabled = ini_getbool("LOG", "log", 1, CONFIG_PATH);
    log_set_enabled(log_enabled);
    
    // 初始化配置路径
    if (!InitializeConfigPath()) {
        // 失败则退出程序
        loop_error = true;
        return;
    }
    
    // 创建并启动IPC服务
    ipc_server = new IPCServer();
    
    if (!ipc_server->Start("keyLoop")) {
        log_error("IPC服务启动失败！");
        delete ipc_server;
        ipc_server = nullptr;
        loop_error = true;
        return;
    }

    // 设置IPC退出回调：当收到退出命令时，标记主循环退出
    ipc_server->SetExitCallback([this]() {
        log_info("IPC请求退出连发系统模块！");
        loop_error = true;
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
    // 主程序循环 - 监控游戏状态，自动启动/停止连发模块
    while (!loop_error) {
        // 获取当前游戏 Title ID
        u64 current_tid = GetCurrentGameTitleId();
        
        // 检测 Title ID 是否发生变化
        if (current_tid != last_game_tid) {
            log_info("检测到游戏状态变化: 0x%016lX -> 0x%016lX", last_game_tid, current_tid);
            
            if (current_tid != 0) {
                // 检测到游戏启动
                log_info("检测到游戏启动，自动开启连发模块");
                StartAutoKey();
            } else {
                // 检测到游戏退出
                log_info("检测到游戏退出，自动关闭连发模块");
                StopAutoKey();
            }
            
            // 更新上次检测的 Title ID
            last_game_tid = current_tid;
        }
        
        // 每1秒检测一次
        svcSleepThread(1000000000ULL);
    }
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
    // 如果已经创建，则不重复创建
    if (autokey_manager != nullptr) {
        log_warning("请勿重复创建连发模块！");
        return true;
    }
    
    // 创建并初始化连发模块
    autokey_manager = new AutoKeyManager();
    
    if (autokey_manager == nullptr) {
        log_error("连发模块创建失败！");
        return false;
    }
    
    return true;
}

// 退出连发模块
void App::StopAutoKey() {
    // 如果没有创建，则无需清理
    if (autokey_manager == nullptr) {
        log_warning("连发模块未运行，无需停止！");
        return;
    }
    
    // 清理连发模块
    delete autokey_manager;
    autokey_manager = nullptr;
    log_info("连发模块已停止！");
}