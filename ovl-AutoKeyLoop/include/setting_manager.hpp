#pragma once
#include <tesla.hpp>





// 游戏设置界面
class GameSetting : public tsl::Gui 
{
public:
    GameSetting();  // 构造函数
    
    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
};

// 全局设置界面
class GlobalSetting : public tsl::Gui 
{
public:
    GlobalSetting();  // 构造函数
    
    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
};

// 总设置界面
class AutoKeySetting : public tsl::Gui 
{
public:
    AutoKeySetting();  // 构造函数
    
    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
};


