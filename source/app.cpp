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
    bool log_enabled = ini_getbool("LOG", "log", 0, CONFIG_PATH);
    log_set_enabled(log_enabled);
    
    // 初始化配置路径
    if (!InitializeConfigPath()) {
        // 失败则退出程序
        loop_error = true;
        return;
    }
    
    // 测试代码，暂时不初始化连发模块，避免影响测试（AI请勿修改）
    // 创建并初始化连发模块
    // autokey_manager = new AutoKeyManager();
}

App::~App() {
    // 清理AutoKeyManager
    if (autokey_manager) {
        delete autokey_manager;
        autokey_manager = nullptr;
    }
}

void App::Loop() {
    // 主程序循环 - AutoKeyManager的线程已经在后台运行
    while (!loop_error) {
        // 保持主线程运行
        svcSleepThread(1000000000ULL);  // 1秒
    }
}