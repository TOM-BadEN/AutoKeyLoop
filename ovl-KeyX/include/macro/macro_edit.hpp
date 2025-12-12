#pragma once
#include <tesla.hpp>

class MacroEditGui : public tsl::Gui 
{
public:
    MacroEditGui(const char* macroFilePath, const char* gameName);
    ~MacroEditGui();
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    const char* m_macroFilePath;
    const char* m_gameName;
    
    s32 m_scrollOffset = 0;
    s32 m_selectedIndex = 0;
    s32 m_listHeight = 0;  // 列表可见区域高度（绘制时更新）
    u32 m_repeatCounter = 0;  // 按键重复计数器
    
    // 选择模式
    bool m_selectMode = false;
    s32 m_selectAnchor = -1;  // 选择起点（按A时的位置）
    
    // 时长编辑模式
    bool m_durationEditMode = false;
    s32 m_editingDuration = 0;  // 编辑中的时长（毫秒）
    s32 m_editDigitIndex = 0;   // 当前编辑的位 (0=千位, 1=百位, 2=十位, 3=个位)
    
    // 按键编辑模式
    bool m_buttonEditMode = false;
    s32 m_buttonEditIndex = 0;  // 光标位置 (0-15)
    u64 m_editingButtons = 0;   // 编辑中的按键掩码
    
    // 菜单模式
    bool m_menuMode = false;
    s32 m_menuIndex = 0;      // 当前选中的菜单项 (0~4)
    s32 m_menuSelStart = 0;   // 菜单模式下的选中范围起始
    s32 m_menuSelEnd = 0;     // 菜单模式下的选中范围结束
    
    bool isMenuItemEnabled(int index) const;
    
    static constexpr s32 TIMELINE_HEIGHT = 100;
    static constexpr s32 ITEM_HEIGHT = 70;  // 和 tsl::style::ListItemDefaultHeight 一致
    static constexpr u32 REPEAT_DELAY = 20;  // 首次触发后延迟帧数
    static constexpr u32 REPEAT_RATE = 8;    // 重复触发间隔帧数
};
