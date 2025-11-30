#pragma once

#include <tesla.hpp>

// 脚本查看类
class MacroViewGui : public tsl::Gui {
public:
    MacroViewGui(const char* macroFilePath, const char* gameName, bool isRecord = false);
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
private:

    struct MacroInfo {
        char gameName[64];
        char titleId[17];
        char macroName[64];
        char fileSizeKb[16];
        char durationSec[8];
        char fps[16];
        char totalFrames[8];
    };

    MacroInfo m_info{};
    char m_macroFilePath[96];
    bool m_isRecord;
    u64 m_Hotkey = 0;
    char m_gameCfgPath[96];
    tsl::elm::ListItem* m_deleteItem = nullptr;
    tsl::elm::ListItem* m_listButton = nullptr;

    void ParsingMacros();
    void getHotkey();

};


// 设置快捷键
class MacroHotKeySettingGui : public tsl::Gui {

public:
    MacroHotKeySettingGui(const char* macroFilePath, const char* gameName, const char* gameCfgPath, u64 Hotkey);
    virtual void update() override;
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    
private:
    bool isHotkeyValid() const;
    
    char m_macroFilePath[96];
    char m_gameName[64];
    char m_gameCfgPath[96];
    u64 m_Hotkey;
    std::vector<u64> m_usedHotkeys{};  // 已使用的快捷键列表
    std::string m_displayText;

    tsl::elm::ListItem* m_HotKeyNO1 = nullptr;
    tsl::elm::ListItem* m_HotKeyNO2 = nullptr;
    tsl::elm::ListItem* m_HotKeyNO3 = nullptr;
    tsl::elm::ListItem* m_HotKeySave = nullptr;
    tsl::elm::ListItem* m_HotKeyDelete = nullptr;
};


// 按键选择
class ButtonSelectGui : public tsl::Gui {
public:
    ButtonSelectGui(int slotIndex);  // 1, 2, 或 3
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    virtual tsl::elm::Element* createUI() override;
    
private:
    int m_slotIndex;
    bool m_canDelete;
};


