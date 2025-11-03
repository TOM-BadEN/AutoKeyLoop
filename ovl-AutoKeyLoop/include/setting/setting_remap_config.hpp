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
    char m_gameName[32];        // 当前游戏名称
    
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
    SettingRemapEdit(int buttonIndex, int currentIndex = 0);
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
private:
    int m_buttonIndex;          // 要修改的按键索引（0-15）
    int m_currentIndex;         // 当前滚轮位置（0-15）
    bool m_needRefresh = false; // 需要刷新标志
    
    // 保存列表项指针数组，用于动态更新
    static constexpr int VISIBLE_ITEMS = 7;
    tsl::elm::ListItem* m_listItems[VISIBLE_ITEMS] = {nullptr};
    
    // 更新所有列表项的显示
    void updateListItems();
};

