#pragma once
#include <tesla.hpp>

// 定义应用版本号
#define APP_VERSION_STRING "v1.0.0"

// 关于插件界面类
class AboutPlugin : public tsl::Gui 
{
public:
    // 构造函数
    AboutPlugin();
    
    // 创建UI界面
    virtual tsl::elm::Element* createUI() override;
};