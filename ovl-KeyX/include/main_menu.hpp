#pragma once
#include <tesla.hpp>
#include "about.hpp"  // 包含关于插件界面头文件

// 游戏状态结构体
struct KeyXInfo {
    bool isInGame;                      // 是否在游戏中
    char gameId[17];                    // 游戏ID
    bool isGlobalConfig;                // 全局配置还是自定义配置
    bool isAutoFireEnabled;             // 连发开关
    std::string GameConfigPath;         // 配置文件路径
    u64 buttons;                        // 连发按键
    bool isAutoRemapEnabled;            // 映射开关
    bool isAutoMacroEnabled;            // 宏开关
    u64 macroHotKey;                    // 宏快捷键
};

// 主菜单类定义
class MainMenu : public tsl::Gui 
{
public:
    MainMenu();  // 构造函数

    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:

    KeyXInfo m_KeyXinfo{};
    tsl::elm::ListItem* m_AutoFireEnableItem = nullptr;
    tsl::elm::ListItem* m_AutoRemapEnableItem = nullptr;
    tsl::elm::ListItem* m_AutoMacroEnableItem = nullptr;
    u64 m_macroHotKey = 0;

    void RefreshData();     // 更新数据
    void AutoKeyToggle();   // 连发功能开关
    void AutoRemapToggle(); // 映射功能开关
    void AutoMacroToggle(); // 宏功能开关
    void ConfigToggle();    // 配置切换（全局/独立）

};