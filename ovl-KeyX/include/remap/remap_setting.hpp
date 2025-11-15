#pragma once
#include <tesla.hpp>

class SettingRemap : public tsl::Gui 
{
public:
    SettingRemap();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;

private:
    bool m_defaultAuto = false;
    tsl::elm::ListItem* m_listIndependentSetting = nullptr;
};


