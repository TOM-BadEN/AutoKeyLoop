#pragma once
#include <tesla.hpp>

class SettingTurbo : public tsl::Gui 
{
public:
    SettingTurbo();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;

private:
    bool m_defaultAuto = false;
    bool m_isJCRightHand = true;
    tsl::elm::ListItem* m_listIndependentSetting = nullptr;
};

