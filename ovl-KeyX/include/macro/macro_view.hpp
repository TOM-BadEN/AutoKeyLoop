#pragma once

#include <tesla.hpp>

// 脚本查看类
class MacroViewGui : public tsl::Gui {
public:
    MacroViewGui(const char* macroFilePath, const char* gameName);
    virtual tsl::elm::Element* createUI() override;
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
    const char* m_macroFilePath;

    // 删除按钮
    tsl::elm::ListItem* m_deleteItem = nullptr;


    void ParsingMacros();

};


