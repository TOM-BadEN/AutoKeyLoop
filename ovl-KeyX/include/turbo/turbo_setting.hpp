#pragma once
#include <tesla.hpp>

class SettingTurbo : public tsl::Gui 
{
public:
    SettingTurbo();
    virtual tsl::elm::Element* createUI() override;
private:
    bool m_defaultAuto = false;
};

