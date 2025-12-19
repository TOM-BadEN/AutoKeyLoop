#pragma once

#include <switch.h>
#include "common.hpp"

class Turbo {
public:
    Turbo(const char* config_path);
    
    // 加载配置
    void LoadConfig(const char* config_path);
    
    // 核心函数：处理输入，填充处理结果（事件+按键数据）
    void Process(ProcessResult& result, bool isJoyCon);
    
    // 重置连发状态（用于暂停时清理）
    void TurboFinishing();

    // 获取只允许左边还是右边的手柄联发
    bool IsJCRightHand();

private:

    bool m_isJCRightHand = true;
    // 配置参数
    u64 m_ButtonMask;           // 连发按键白名单
    u64 m_PressDurationNs;      // 按下持续时间（纳秒）
    u64 m_ReleaseDurationNs;    // 松开持续时间（纳秒）
    
    // 运行状态
    bool m_IsActive;            // 是否运行中
    bool m_IsPressed;           // 当前是按下还是松开周期
    u64 m_TurboStartTime;       // 连发开始时间
    u64 m_InitialPressTime;     // 首次按下时间（用于200ms延迟）
    bool m_DelayStart;          // 是否启用延迟启动
    
    // 事件判定
    FeatureEvent DetermineEvent(u64 autokey_buttons);
    
    // 事件处理
    void TurboStarting();
    void TurboExecuting(u64 autokey_buttons, u64 normal_buttons, ProcessResult& result);
    
    // 辅助函数
    bool CheckRelease(u64 autokey_buttons);
};


