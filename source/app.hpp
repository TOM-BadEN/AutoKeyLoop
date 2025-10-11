#pragma once
#include "input_monitor.hpp"

namespace MP {
    // APP应用程序类
    class App final {
    private:
        InputMonitor* input_monitor;  // 输入监控器
        
        // 处理按键事件
        void HandleButtonEvent(const ButtonEvent& event);
        
    public:
        // 构造函数
        App();
        
        // 析构函数
        ~App();
        
        // 主循环函数
        void Loop();
    };
}