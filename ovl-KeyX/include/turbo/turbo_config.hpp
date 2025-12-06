#pragma once
#include <tesla.hpp>

class SettingTurboConfig : public tsl::Gui 
{
public:
    SettingTurboConfig(bool isGlobal, u64 currentTitleId);
    virtual tsl::elm::Element* createUI() override;
    
private:
    bool m_isGlobal;  // true=全局配置, false=独立配置
    char m_gameName[64];  // 当前游戏名称
    char m_ConfigPath[256];  // 配置文件路径
    bool m_TurboSpeed;  // 是否高速连发
    bool m_DelayStart;  // 是否延迟启动
};

// 按键设置界面
class SettingTurboButton : public tsl::Gui 
{
public:
    SettingTurboButton(const char* configPath, bool isGlobal);
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    virtual void update() override;
    
private:
    const char* m_configPath;   // 配置文件路径（只读，不需要复制）
    bool m_isGlobal;            // true=全局配置, false=独立配置
    std::vector<tsl::elm::ToggleListItem*> m_toggleItems;
};

