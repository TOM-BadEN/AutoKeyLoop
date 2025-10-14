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
private:
    // 配置值成员变量
    std::string m_FireInterval;   // 连发间隔时间（ms）
    std::string m_PressTime;      // 按住持续时间（ms）
    
public:
    GlobalSetting();  // 构造函数
    
    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
};

// 时间设置界面（通用：用于设置松开时间和按住时间）
class TimeSetting : public tsl::Gui 
{
private:
    int m_currentTime;              // 当前选中的时间值（会在createUI中转换为索引）
    std::string m_configKey;        // 配置键名 ("fireinterval" 或 "presstime")
    std::string m_title;            // 界面标题
    bool m_isGlobal;                // 是否全局设置
    
    tsl::elm::List* m_list;         // 列表指针（用于延迟焦点设置）
    bool m_needsRefocus;            // 是否需要重新设置焦点
    int m_frameCounter;             // 帧计数器（延迟2帧后设置焦点）
    
public:
    TimeSetting(std::string currentValue, std::string configKey, 
                std::string title, bool isGlobal);
    
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
};

// 总设置界面
class AutoKeySetting : public tsl::Gui 
{
private:
    // 配置值成员变量
    bool m_logEnabled;    // 日志开关
    bool m_autoEnabled;   // 自动开启连发
    
public:
    AutoKeySetting();  // 构造函数
    
    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
};


