#pragma once

#include <switch.h>
#include "common.hpp"
#include <vector>

class Macro {
public:
    Macro(const char* macroCfgPath);
    
    // 加载配置
    void LoadConfig(const char* macroCfgPath);
    
    // 核心函数：处理输入，填充处理结果（事件+按键数据）
    void Process(ProcessResult& result);
    
    // 宏结束清理工作
    void MacroFinishing();                    

private:

    struct MacroEntry {
        char MacroFilePath[96];    // 宏文件路径
        u64 combo;                 // 快捷键组合
    };

    // 宏文件头
    struct MacroHeader {
        char magic[4];      // "KEYX"
        u16 version;        // 版本号
        u16 frameRate;      // 帧率
        u64 titleId;        // 游戏TID
        u32 frameCount;     // 总帧数
    } __attribute__((packed));

    // 宏单帧数据
    struct MacroFrame {
        u64 keysHeld;       // 按键状态
        s32 leftX;          // 左摇杆X
        s32 leftY;          // 左摇杆Y
        s32 rightX;         // 右摇杆X
        s32 rightY;         // 右摇杆Y
    } __attribute__((packed));

    // 宏单帧数据V2（1.4.1新增）
    // 为了解决帧率不稳导致复现播放不精确的问题，新增时间戳字段
    struct MacroFrameV2 {
        u32 durationMs;     // 持续时间（毫秒）
        u64 keysHeld;       // 按键状态
        s32 leftX;          // 左摇杆X
        s32 leftY;          // 左摇杆Y
        s32 rightX;         // 右摇杆X
        s32 rightY;         // 右摇杆Y
    } __attribute__((packed));

    std::vector<MacroEntry> m_Macros{};     // 宏列表
    bool m_IsPlaying = false;               // 是否正在播放
    int m_CurrentMacroIndex = -1;           // 当前播放的宏索引
    u32 m_CurrentFrameIndex = 0;            // 当前播放的帧索引
    u64 m_PlaybackStartTick = 0;            // 播放开始时间
    u16 m_FrameRate = 0;                    // 宏帧率
    u16 m_Version = 1;                      // 宏版本
    std::vector<MacroFrame> m_Frames{};     // V1 宏帧数据
    std::vector<MacroFrameV2> m_FramesV2{}; // V2 宏帧数据
    bool m_HotkeyPressed = false;           // 上一次快捷键状态
    u64 m_HotkeyPressTime = 0;              // 快捷键按下时间
    bool m_RepeatMode = false;              // 循环播放标志
    u64 m_LastFinishTime = 0;               // 上次停止的时间
    bool m_JustStopped = false;             // 刚停止，等待冷静期
    u64 m_AccumulatedMs = 0;                // V2 播放累加时间
    bool m_MacroHasStick = false;           // 当前宏是否包含摇杆操作

    FeatureEvent DetermineEvent(u64 buttons);         // 判定事件
    FeatureEvent HandlePlayingState(u64 buttons);     // 处理播放中状态
    FeatureEvent HandleStopCooldown(u64 buttons);     // 处理停止后冷静期
    FeatureEvent HandleNormalTrigger(u64 buttons);    // 处理正常触发检测
    int CheckHotkeyTriggered(u64 buttons);            // 检查快捷键触发
    u32 CalculateTargetFrame();                       // 计算当前应该播放第几帧
    void MacroStarting();                             // 宏启动
    void LoadMacroFile(const char* filePath);         // 加载宏文件
    void MacroExecuting(ProcessResult& result);       // 宏执行
    
};
