#pragma once
#include <tesla.hpp>





// 游戏设置界面
class GameSetting : public tsl::Gui 
{
private:
    std::string m_titleId;            // 当前游戏 Title ID（0表示不在游戏）
    std::string m_gameConfigPath;      // 当前游戏配置文件路径
    // 速度选项成员变量
    bool m_IsHighSpeed;              // 是否高速连发
    tsl::elm::ListItem* m_HighSpeedItem;  // 高速连发列表项
    tsl::elm::ListItem* m_LowSpeedItem;   // 普通连发列表项
    
    
public:
    GameSetting(std::string titleId);  // 构造函数
    
    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
};

// 全局设置界面
class GlobalSetting : public tsl::Gui 
{
private:
    // 速度选项成员变量
    bool m_IsHighSpeed;              // 是否高速连发
    tsl::elm::ListItem* m_HighSpeedItem;  // 高速连发列表项
    tsl::elm::ListItem* m_LowSpeedItem;   // 普通连发列表项
    
public:
    GlobalSetting();  // 构造函数
    
    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
};

// 按键设置界面
class ButtonSetting : public tsl::Gui 
{
private:
    u64 m_selectedButtons;          // 当前选中的按键（位掩码，与HidNpadButton兼容）
    std::string m_configPath;       // 配置文件路径
    bool m_isGlobal;                // 是否全局设置
    
public:
    ButtonSetting(u64 currentButtons, std::string configPath, bool isGlobal);
    
    virtual tsl::elm::Element* createUI() override;
};


// 总设置界面
class AutoKeySetting : public tsl::Gui 
{
private:
    std::string m_titleId;            // 当前游戏 Title ID（0表示不在游戏）
    std::string m_gameName;            // 当前游戏 Name
    bool m_logEnabled;        // 日志开关
    bool m_autoEnabled;       // 自动开启连发
    bool m_notifEnabled;      // 通知开关
    
public:
    AutoKeySetting(std::string titleId);  // 构造函数
    
    virtual tsl::elm::Element* createUI() override;  // 创建用户界面
};


