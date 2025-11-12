#pragma once
#include <tesla.hpp>
#include "about.hpp"  // 包含关于插件界面头文件

// 游戏状态结构体
struct TextAreaInfo {
    bool isInGame;                      // 是否在游戏中
    char gameId[17];                    // 游戏ID
    bool isGlobalConfig;                // 全局配置还是自定义配置
    bool isAutoFireEnabled;             // 连发开关
    std::string GameConfigPath;         // 配置文件路径
    u64 buttons;                        // 连发按键
    bool isAutoRemapEnabled;            // 映射开关
};

// 主菜单类定义
class MainMenu : public tsl::Gui 
{
public:
    MainMenu();  // 构造函数

    virtual tsl::elm::Element* createUI() override;  // 创建用户界面

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
    // 静态方法：更新游戏信息
    static void UpdateMainMenu();
    
    // 静态方法：刷新按钮配置
    static void RefreshButtonsConfig();
    
    // 静态方法：连发功能开关
    static void AutoKeyToggle();

    // 静态方法：映射功能开关
    static void AutoRemapToggle();
    
    // 静态方法：配置切换（全局/独立）
    static void ConfigToggle();
    
    // 静态成员变量：文本区域信息
    static TextAreaInfo s_TextAreaInfo;
    

};