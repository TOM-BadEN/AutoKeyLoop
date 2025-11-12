#pragma once
#include <tesla.hpp>

class SettingRemap : public tsl::Gui 
{
public:
    SettingRemap();
    virtual tsl::elm::Element* createUI() override;

private:
    bool m_defaultAuto = false;
};


