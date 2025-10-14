#include "setting_manager.hpp"
#include "sysmodule_manager.hpp"

// ========== 游戏设置界面 ==========

// 构造函数
GameSetting::GameSetting()
{
}

// 创建用户界面
tsl::elm::Element* GameSetting::createUI()
{
    // 创建根框架
    auto rootFrame = new tsl::elm::OverlayFrame("游戏设置", "仅针对当前游戏生效");
    
    // 创建列表
    auto list = new tsl::elm::List();
    
    // 添加列表项（暂时空着）
    auto listItem1 = new tsl::elm::ListItem("设置项1", ">");
    list->addItem(listItem1);
    
    auto listItem2 = new tsl::elm::ListItem("设置项2", ">");
    list->addItem(listItem2);
    
    auto listItem3 = new tsl::elm::ListItem("设置项3", ">");
    list->addItem(listItem3);
    
    // 将列表添加到框架
    rootFrame->setContent(list);
    
    return rootFrame;
}

// ========== 全局设置界面 ==========

// 构造函数
GlobalSetting::GlobalSetting()
{
}

// 创建用户界面
tsl::elm::Element* GlobalSetting::createUI()
{
    // 创建根框架
    auto rootFrame = new tsl::elm::OverlayFrame("全局设置", "适应所有游戏");
    
    // 创建列表
    auto list = new tsl::elm::List();
    
    // 添加分类标题
    auto categoryHeader1 = new tsl::elm::CategoryHeader(" 彻底关闭/开启系统模块");
    list->addItem(categoryHeader1);
    
    // 添加列表项（暂时空着）
    bool isModuleRunning = SysModuleManager::isRunning();
    auto listItem1 = new tsl::elm::ListItem("连发模块", isModuleRunning ? "开" : "关");
    list->addItem(listItem1);

    // 添加分类标题
    auto categoryHeader3 = new tsl::elm::CategoryHeader(" 游戏默认开启连发功能");
    list->addItem(categoryHeader3);
    
    // 添加列表项（暂时空着）
    auto listItem6 = new tsl::elm::ListItem("自动开启", "开");
    list->addItem(listItem6);

    // 添加分类标题
    auto categoryHeader2 = new tsl::elm::CategoryHeader(" 连发参数设置");
    list->addItem(categoryHeader2);

    auto listItem4 = new tsl::elm::ListItem("连发按键", ">");
    list->addItem(listItem4);
    
    auto listItem2 = new tsl::elm::ListItem("连发间隔", ">");
    list->addItem(listItem2);
    
    auto listItem3 = new tsl::elm::ListItem("按住时间", ">");
    list->addItem(listItem3);
    
    
    // 将列表添加到框架
    rootFrame->setContent(list);
    
    return rootFrame;
}

