#pragma once

#include <tesla.hpp>
#include <string>
#include "macro_util.hpp"

// 脚本查看类
class MacroViewGui : public tsl::Gui {
public:
    MacroViewGui(const char* macroFilePath, const char* gameName, bool isRecord = false);
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
private:
    char m_gameName[64];
    char m_macroFilePath[128];
    bool m_isRecord;
    u64 m_Hotkey = 0;
    tsl::elm::ListItem* m_listButton = nullptr;

    void getHotkey();

};


// 设置快捷键
class MacroHotKeySettingGui : public tsl::Gui {

public:
    MacroHotKeySettingGui(const char* macroFilePath, const char* gameName, u64 Hotkey);
    virtual void update() override;
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
private:
    bool isHotkeyValid() const;
    bool handleEditInput(u64 keysDown);

    char m_macroFilePath[96];
    char m_gameName[64];
    u64 m_titleId;
    u64 m_Hotkey;
    std::vector<u64> m_usedHotkeys{};  // 已使用的快捷键列表
    std::string m_displayText;
    
    // 编辑模式
    bool m_editMode = false;
    s32 m_cursorIndex = 0;
    u64 m_selectedButtons = 0;

    tsl::elm::ListItem* m_HotKeySetting = nullptr;  // 设置按键（合并原来的三个）
    tsl::elm::ListItem* m_HotKeySave = nullptr;
    tsl::elm::ListItem* m_HotKeyDelete = nullptr;
};
