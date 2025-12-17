#pragma once
#include <tesla.hpp>

class MacroRenameGui : public tsl::Gui 
{
public:
    MacroRenameGui(const char* macroFilePath, const char* gameName, bool isRecord = false);
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    char m_macroFilePath[96];
    char m_gameName[64];
    bool m_isRecord;
    int m_row = 0;
    int m_col = 0;
    char m_input[32];
    char m_inputOld[32];
    size_t m_inputLen = 0;
    u32 m_cursorBlink = 0;
    u16 m_backspaceTick = 0;
    u16 m_dirTick = 0;
    HidNpadButton m_lastDir = (HidNpadButton)0;
    bool m_useLower = true;
    bool m_isDuplicate = false;

    // 改名函数
    void MacroRename();
    
    // 检测重名
    void checkDuplicate();
};
