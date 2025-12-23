#pragma once
#include <tesla.hpp>
#include "macro_util.hpp"

// 脚本详情界面
class MacroDetailGui : public tsl::Gui {
public:
    MacroDetailGui(const std::string& macroFilePath, const std::string& gameName);
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    void drawDetail(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);

    std::string m_filePath;
    std::string m_gameName;
    MacroUtil::MacroMetadata m_meta;
    
    s32 m_scrollOffset = 0;
    s32 m_maxScrollOffset = 0;
};
