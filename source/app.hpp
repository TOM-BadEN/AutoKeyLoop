#pragma once
#include "autokeymanager.hpp"
#include "ipc.hpp"  // ← 添加IPC头文件

// APP应用程序类
class App final {
private:

    AutoKeyManager* autokey_manager = nullptr;  // 自动按键管理器
    IPCServer* ipc_server = nullptr;            // IPC服务器 ← 新增

    // 控制住循环是否结束的
    bool loop_error = false;
    
    // 初始化配置路径（确保目录存在）
    bool InitializeConfigPath();
    
public:
    // 构造函数
    App();
    
    // 析构函数
    ~App();
    
    // 主循环函数
    void Loop();
};