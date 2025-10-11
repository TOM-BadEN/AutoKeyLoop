#include <switch.h>
#include "app.hpp"
#include "../log/log.h"

namespace MP {
    // App类的实现
    App::App() {
        // 构造函数 - 创建并初始化输入监控器
        input_monitor = new InputMonitor();
        input_monitor->Initialize();
        
        // 设置事件回调函数
        input_monitor->SetEventCallback([this](const ButtonEvent& event) {
            HandleButtonEvent(event);
        });
    }

    App::~App() {
        // 析构函数 - 清理输入监控器
        if (input_monitor) {
            delete input_monitor;
            input_monitor = nullptr;
        }
    }

    void App::Loop() {
        // 主程序循环
        if (input_monitor) log_info("系统模块启动，开始监控按键状态!");
        else log_error("按键监控初始化失败！");

        while (true) {
            // 更新输入监控状态
            if (input_monitor) {
                input_monitor->Update();
            }
            
            // 线程休眠约16.67ms (60FPS)
            svcSleepThread(16666667);
        }
    }
    
    // 处理按键事件
    void App::HandleButtonEvent(const ButtonEvent& event) {
        switch (event.type) {
            case ButtonEventType::PRESS:
                log_info("按下%s", event.button_name);
                break;
                
            case ButtonEventType::HOLD:
                log_info("长按%s", event.button_name);
                // 未来连发模块: auto_fire->StartAutoFire(event.button_name);
                break;
                
            case ButtonEventType::RELEASE:
                log_info("松开%s", event.button_name);
                // 未来连发模块: auto_fire->StopAutoFire(event.button_name);
                break;
        } 
    }
}