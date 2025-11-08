#pragma once

#include <switch.h>
#include "common.hpp"

class Macro {
public:
    Macro(const char* config_path);
    
    // 加载配置
    void LoadConfig(const char* config_path);
    
    // 核心函数：处理输入，填充处理结果（事件+按键数据）
    void Process(ProcessResult& result);
    
    // 重置宏状态（用于暂停时清理）
    void MacroFinishing();

private:
    // TODO: 宏配置和状态
};
