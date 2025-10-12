#include <switch.h>
#include "app.hpp"
#include "../log/log.h"

namespace MP {
    // App类的实现
    App::App() {
        log_info("系统模块启动 - 自动连发已启用");
        
        // 创建并初始化连发模块
        autokey_manager = new AutoKeyManager();
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
        while (true) {
            // 保持主线程运行
            svcSleepThread(1000000000ULL);  // 1秒
        }
    }
}