#pragma once
#include <switch.h>

// 功能事件（连发和宏共用）
enum class FeatureEvent {
    PAUSED,       // 模块暂停0
    IDLE,         // 无操作/待机1
    STARTING,     // 启动功能2
    EXECUTING,    // 执行中3
    FINISHING     // 功能结束4
};

// 处理结果（连发和宏共用）
struct ProcessResult {
    FeatureEvent event;                     // 事件状态
    u64 buttons;                            // 原始物理输入的按键
    HidAnalogStickState analog_stick_l;     // 左摇杆
    HidAnalogStickState analog_stick_r;     // 右摇杆
    u64 OtherButtons;                       // 修改后的完整按键（给Pro/Lite用）
    u64 JoyconButtons;                      // 修改后的连发按键（给JoyCon用）
};

