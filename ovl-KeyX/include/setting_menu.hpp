#pragma once
#include <tesla.hpp>

class SettingMenu : public tsl::Gui 
{
public:
    SettingMenu();
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
private:
    bool m_notifEnabled = false;
    bool m_running = false;
    bool m_hasFlag = false;
};

