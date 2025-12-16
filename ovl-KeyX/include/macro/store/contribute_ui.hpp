#pragma once
#include <tesla.hpp>

/**
 * 投稿界面 - 显示投稿方式和联系信息
 */
class ContributeGui : public tsl::Gui {
public:
    ContributeGui();
    virtual tsl::elm::Element* createUI() override;
};
