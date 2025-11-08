#include "macro.hpp"

// 构造函数
Macro::Macro(const char* config_path) {
    // TODO: 初始化宏模块
    LoadConfig(config_path);
}

// 加载配置
void Macro::LoadConfig(const char* config_path) {
    // TODO: 从配置文件加载宏序列
}

// 核心函数：处理输入
void Macro::Process(ProcessResult& result) {
    // TODO: 实现宏处理逻辑
    // 目前直接返回 IDLE，不做任何处理
    result.event = FeatureEvent::IDLE;
}

// 重置宏状态
void Macro::MacroFinishing() {
    // TODO: 重置宏状态机
}
