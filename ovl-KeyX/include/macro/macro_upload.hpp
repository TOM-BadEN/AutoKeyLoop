#pragma once
#include <tesla.hpp>
#include "macro_util.hpp"

// 脚本详情界面
class MacroDetailGui : public tsl::Gui {
public:
    MacroDetailGui(const char* macroFilePath, const char* gameName);
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    char m_gameName[64];
    char m_filePath[96];
    MacroUtil::MacroMetadata m_meta;
};
