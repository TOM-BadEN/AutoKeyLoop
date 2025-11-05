#pragma once
#include <tesla.hpp>

class SettingRemapConfig : public tsl::Gui 
{
public:
    SettingRemapConfig(bool isGlobal, u64 currentTitleId);
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
private:
    char m_gameName[64];        // 当前游戏名称
    
    void loadMappings();        // 读取映射配置
};

// 映射布局显示界面
class SettingRemapDisplay : public tsl::Gui 
{
public:
    SettingRemapDisplay();
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
private:
    void resetMappings();       // 重置映射
};

// 按键映射编辑界面
class SettingRemapEdit : public tsl::Gui 
{
public:
    SettingRemapEdit(int buttonIndex);
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
private:
    int m_buttonIndex;          // 要修改的按键索引（0-15）
    bool m_needRefresh = false; // 需要刷新标志
};

