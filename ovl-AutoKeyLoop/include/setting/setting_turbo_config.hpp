#pragma once
#include <tesla.hpp>

class SettingTurboConfig : public tsl::Gui 
{
public:
    SettingTurboConfig(bool isGlobal, u64 currentTitleId);
    virtual tsl::elm::Element* createUI() override;
    
private:
    bool m_isGlobal;  // true=全局配置, false=独立配置
    char m_gameName[32];  // 当前游戏名称
    char m_ConfigPath[256];  // 配置文件路径
    bool m_TurboSpeed;  // 是否高速连发
};

// 按键设置界面
class SettingTurboButton : public tsl::Gui 
{
public:
    SettingTurboButton(const char* configPath, bool isGlobal);
    virtual tsl::elm::Element* createUI() override;
    
private:
    const char* m_configPath;   // 配置文件路径（只读，不需要复制）
    bool m_isGlobal;            // true=全局配置, false=独立配置
};

