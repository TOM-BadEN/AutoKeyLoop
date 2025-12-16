#pragma once
#include <tesla.hpp>

/**
 * 脚本商店界面
 */
class StoreGui : public tsl::Gui {
public:
    StoreGui();
    virtual tsl::elm::Element* createUI() override;
};
