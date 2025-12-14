#pragma once
#include <tesla.hpp>

class UpdaterUI : public tsl::Gui 
{
public:
    UpdaterUI();
    virtual tsl::elm::Element* createUI() override;
};
