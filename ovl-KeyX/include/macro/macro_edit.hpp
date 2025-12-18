#pragma once
#include <tesla.hpp>
#include "macro_data.hpp"

class MacroEditGui : public tsl::Gui 
{
public:
    MacroEditGui(const char* gameName, bool isRecord);
    ~MacroEditGui();
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    const char* m_gameName;
    bool m_isRecord;
    bool m_bakMacro = false;
    
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
    
    // 摇杆编辑模式
    bool m_stickEditMode = false;
    s32 m_stickEditIndex = 0;   // 光标位置 (0-15, 0-7=左摇杆, 8-15=右摇杆)
    StickDir m_editingStickL = StickDir::None;  // 编辑中的左摇杆方向
    StickDir m_editingStickR = StickDir::None;  // 编辑中的右摇杆方向
    
    // 菜单模式
    bool m_menuMode = false;
    s32 m_menuIndex = 0;      // 当前选中的菜单项
    s32 m_menuLevel = 0;      // 菜单层级 (0=一级, 1=二级)
    s32 m_parentIndex = -1;   // 进入子菜单前的一级菜单索引
    s32 m_menuSelStart = 0;   // 菜单模式下的选中范围起始
    s32 m_menuSelEnd = 0;     // 菜单模式下的选中范围结束
    
    bool hasVirtualMergedStick() const;
    bool hasStickL() const;
    bool hasStickR() const;
    bool isMenuItemEnabled(int index) const;
    
    // 绘制方法
    void drawButtonEditArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress);
    void drawDurationEditArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress);
    void drawStickEditArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress);
    void drawMenuArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress);
    void drawTimelineArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress);
    void drawActionList(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h, double progress);
    bool handleButtonEditInput(u64 keysDown);
    bool handleDurationEditInput(u64 keysDown);
    bool handleStickEditInput(u64 keysDown);
    bool handleMenuInput(u64 keysDown);
    bool handleNormalInput(u64 keysDown, u64 keysHeld);
    void executeDeleteAction();
    void enterDurationEditMode();
    void enterButtonEditMode();
    void enterStickEditMode();
};
