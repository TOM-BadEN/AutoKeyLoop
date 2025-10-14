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
    auto rootFrame = new tsl::elm::OverlayFrame("独立配置", "仅针对当前游戏生效");
    
    // 创建列表
    auto list = new tsl::elm::List();
    

    // 添加分类标题
    auto categoryHeader2 = new tsl::elm::CategoryHeader(" 连发参数设置");
    list->addItem(categoryHeader2);

    auto listItem4 = new tsl::elm::ListItem("连发按键", ">");
    list->addItem(listItem4);
    
    auto listItem2 = new tsl::elm::ListItem("连发间隔", ">");
    list->addItem(listItem2);
    
    auto listItem3 = new tsl::elm::ListItem("按住时间", ">");
    list->addItem(listItem3);

    // 按键显示区域高度（720 - 标题97 - 底部73 - 分类标题63 - 3个列表项210 = 277）
    s32 buttonDisplayHeight = 277;
    
    // 添加按键布局显示区域
    auto buttonDisplay = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};
        tsl::Color lightBlueColor = {0x00, 0xDD, 0xFF, 0xFF};  // 亮天蓝色
        
        const s32 buttonSize = 30;   // 按钮大小
        const s32 rowSpacing = 10;   // 行间距
        
        // === 水平位置计算 ===
        // 左侧：方向键左距边界40，中心在80
        s32 dpadLeftX = x + 40;
        s32 leftColumnX = x + 80;
        s32 dpadRightX = x + 120;
        
        // 右侧：A键距边界40，中心在w-80
        s32 aButtonX = x + w - 40;
        s32 rightColumnX = x + w - 80;
        s32 yButtonX = x + w - 120;
        
        // === 垂直位置计算 ===
        // 总高度 = 5行×30px + 4个间距×10px = 190px
        const s32 layoutTotalHeight = 5 * buttonSize + 4 * rowSpacing;
        
        // 垂直居中（drawString的y参数是文字底部，需加buttonSize偏移）
        s32 baseY = y + (h - layoutTotalHeight) / 2 + buttonSize;
        
        // === 绘制按钮（5行） ===
        // 行1: ZL, ZR
        s32 row1Y = baseY;
        r->drawString("\uE0A6", false, leftColumnX, row1Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A7", false, rightColumnX, row1Y, buttonSize, a(whiteColor));
        
        // 行2: L, R
        s32 row2Y = baseY + buttonSize + rowSpacing;
        r->drawString("\uE0A4", false, leftColumnX, row2Y, buttonSize, a(lightBlueColor));
        r->drawString("\uE0A5", false, rightColumnX, row2Y, buttonSize, a(whiteColor));
        
        // 行3: 方向键上, X
        s32 row3Y = baseY + 2 * (buttonSize + rowSpacing);
        r->drawString("\uE0AF", false, leftColumnX, row3Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A2", false, rightColumnX, row3Y, buttonSize, a(lightBlueColor));
        
        // 行4: 方向键左右, Y/A
        s32 row4Y = baseY + 3 * (buttonSize + rowSpacing);
        r->drawString("\uE0B1", false, dpadLeftX, row4Y, buttonSize, a(whiteColor));
        r->drawString("\uE0B2", false, dpadRightX, row4Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A3", false, yButtonX, row4Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A0", false, aButtonX, row4Y, buttonSize, a(lightBlueColor));
        
        // 行5: 方向键下, B
        s32 row5Y = baseY + 4 * (buttonSize + rowSpacing);
        r->drawString("\uE0B0", false, leftColumnX, row5Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A1", false, rightColumnX, row5Y, buttonSize, a(whiteColor));
    });
    
    list->addItem(buttonDisplay, buttonDisplayHeight);
    
    
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
    auto rootFrame = new tsl::elm::OverlayFrame("全局配置", "默认使用该配置");
    
    // 创建列表
    auto list = new tsl::elm::List();

    // 添加分类标题
    auto categoryHeader2 = new tsl::elm::CategoryHeader(" 连发参数设置");
    list->addItem(categoryHeader2);

    auto listItem4 = new tsl::elm::ListItem("连发按键", ">");
    list->addItem(listItem4);
    
    auto listItem2 = new tsl::elm::ListItem("连发间隔", ">");
    list->addItem(listItem2);
    
    auto listItem3 = new tsl::elm::ListItem("按住时间", ">");
    list->addItem(listItem3);
    
    // 按键显示区域高度（720 - 标题97 - 底部73 - 分类标题63 - 3个列表项210 = 277）
    s32 buttonDisplayHeight = 277;
    
    // 添加按键布局显示区域
    auto buttonDisplay = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};
        tsl::Color lightBlueColor = {0x00, 0xDD, 0xFF, 0xFF};  // 亮天蓝色
        
        const s32 buttonSize = 30;   // 按钮大小
        const s32 rowSpacing = 10;   // 行间距
        
        // === 水平位置计算 ===
        // 左侧：方向键左距边界40，中心在80
        s32 dpadLeftX = x + 40;
        s32 leftColumnX = x + 80;
        s32 dpadRightX = x + 120;
        
        // 右侧：A键距边界40，中心在w-80
        s32 aButtonX = x + w - 40;
        s32 rightColumnX = x + w - 80;
        s32 yButtonX = x + w - 120;
        
        // === 垂直位置计算 ===
        // 总高度 = 5行×30px + 4个间距×10px = 190px
        const s32 layoutTotalHeight = 5 * buttonSize + 4 * rowSpacing;
        
        // 垂直居中（drawString的y参数是文字底部，需加buttonSize偏移）
        s32 baseY = y + (h - layoutTotalHeight) / 2 + buttonSize;
        
        // === 绘制按钮（5行） ===
        // 行1: ZL, ZR
        s32 row1Y = baseY;
        r->drawString("\uE0A6", false, leftColumnX, row1Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A7", false, rightColumnX, row1Y, buttonSize, a(whiteColor));
        
        // 行2: L, R
        s32 row2Y = baseY + buttonSize + rowSpacing;
        r->drawString("\uE0A4", false, leftColumnX, row2Y, buttonSize, a(lightBlueColor));
        r->drawString("\uE0A5", false, rightColumnX, row2Y, buttonSize, a(whiteColor));
        
        // 行3: 方向键上, X
        s32 row3Y = baseY + 2 * (buttonSize + rowSpacing);
        r->drawString("\uE0AF", false, leftColumnX, row3Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A2", false, rightColumnX, row3Y, buttonSize, a(lightBlueColor));
        
        // 行4: 方向键左右, Y/A
        s32 row4Y = baseY + 3 * (buttonSize + rowSpacing);
        r->drawString("\uE0B1", false, dpadLeftX, row4Y, buttonSize, a(whiteColor));
        r->drawString("\uE0B2", false, dpadRightX, row4Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A3", false, yButtonX, row4Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A0", false, aButtonX, row4Y, buttonSize, a(lightBlueColor));
        
        // 行5: 方向键下, B
        s32 row5Y = baseY + 4 * (buttonSize + rowSpacing);
        r->drawString("\uE0B0", false, leftColumnX, row5Y, buttonSize, a(whiteColor));
        r->drawString("\uE0A1", false, rightColumnX, row5Y, buttonSize, a(whiteColor));
    });
    list->addItem(buttonDisplay, buttonDisplayHeight);
    
    // 将列表添加到框架
    rootFrame->setContent(list);
    
    return rootFrame;
}

// ========== 总设置界面 ==========

// 构造函数
AutoKeySetting::AutoKeySetting()
{
}

// 创建用户界面
tsl::elm::Element* AutoKeySetting::createUI()
{
    // 创建根框架
    auto rootFrame = new tsl::elm::OverlayFrame("连发设置", "插件设置界面");
    
    // 创建列表
    auto list = new tsl::elm::List();
    
    // 添加分类标题
    auto categoryHeader1 = new tsl::elm::CategoryHeader(" 彻底关闭/开启系统模块");
    list->addItem(categoryHeader1);
    
    // 添加连发模块
    bool isModuleRunning = SysModuleManager::isRunning();
    auto listItemModule = new tsl::elm::ListItem("连发模块", isModuleRunning ? "开" : "关");
    list->addItem(listItemModule);

    // 添加分类标题
    auto categoryHeader2 = new tsl::elm::CategoryHeader(" 基础功能设置");
    list->addItem(categoryHeader2);
    
    // 添加默认连发
    auto listItemDefaultAuto = new tsl::elm::ListItem("默认连发", "关");
    list->addItem(listItemDefaultAuto);
    
    // 添加调试日志
    auto listItemDefaultAuto1 = new tsl::elm::ListItem("调试日志", "关");
    list->addItem(listItemDefaultAuto1);

    // 添加分类标题
    auto categoryHeader3 = new tsl::elm::CategoryHeader(" 详细连发参数设置");
    list->addItem(categoryHeader3);

    auto listItemGlobalSetting = new tsl::elm::ListItem("全局配置", ">");
    listItemGlobalSetting->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<GlobalSetting>();
            return true;
        }
        return false;
    });
    list->addItem(listItemGlobalSetting);

    auto listItemIndependentSetting = new tsl::elm::ListItem("独立配置", ">");
    listItemIndependentSetting->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<GameSetting>();
            return true;
        }
        return false;
    });
    list->addItem(listItemIndependentSetting);
    
    // 将列表添加到框架
    rootFrame->setContent(list);
    
    return rootFrame;
}
