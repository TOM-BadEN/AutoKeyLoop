#pragma once
#include "autokeymanager.hpp"

// APP应用程序类
class App final {
private:
    AutoKeyManager* autokey_manager;  // 自动按键管理器
    
public:
    // 构造函数
    App();
    
    // 析构函数
    ~App();
    
    // 主循环函数
    void Loop();
};