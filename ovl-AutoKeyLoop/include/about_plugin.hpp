#pragma once
#include <tesla.hpp>

// 关于插件界面类
class AboutPlugin : public tsl::Gui 
{
public:
    // 构造函数
    AboutPlugin();
    
    // 创建UI界面
    virtual tsl::elm::Element* createUI() override;
};