#pragma once
#include <tesla.hpp>
#include "about_plugin.hpp"  // 包含关于插件界面头文件

// 游戏状态结构体
struct TextAreaInfo {
    bool isInGame;              // 是否在游戏中
    char gameName[64];          // 游戏名字
    char gameId[17];            // 游戏ID
    bool isGlobalConfig;        // 全局配置还是自定义配置
    bool isAutoEnabled;         // 连发开关
    std::string GameConfigPath; // 游戏配置文件路径
};

// 主菜单类定义
class MainMenu : public tsl::Gui 
{
public:
    MainMenu();  // 构造函数

    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
    
    // 静态方法：更新游戏信息
    static void UpdateMainMenu();
    
    // 静态方法：连发功能开关
    static void AutoKeyToggle();
    
    // 静态方法：配置切换（全局/独立）
    static void ConfigToggle();
    
    // 静态成员变量：文本区域信息
    static TextAreaInfo s_TextAreaInfo;
    




};