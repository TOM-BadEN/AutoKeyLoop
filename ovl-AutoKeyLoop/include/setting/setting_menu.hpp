#pragma once
#include <tesla.hpp>

class SettingMenu : public tsl::Gui 
{
public:
    SettingMenu();
    virtual tsl::elm::Element* createUI() override;
private:
    bool m_notifEnabled = false;
    bool m_running = false;
    bool m_hasFlag = false;
};

