#include "setting_manager.hpp"
#include "sysmodule_manager.hpp"
#include "ini_helper.hpp"

#define CONFIG_PATH "/config/AutoKeyLoop/config.ini"

// 文件作用域静态变量 - 只在本文件内的GUI类之间共享
static tsl::elm::ListItem* g_FireIntervalItem = nullptr;
static tsl::elm::ListItem* g_PressTimeItem = nullptr;



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
    
    auto listItem2 = new tsl::elm::ListItem("松开时间", ">");
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
    // 读取连发间隔时间（默认值：100ms）
    m_FireInterval = IniHelper::getString("AUTOFIRE", "fireinterval", "100", CONFIG_PATH);
    
    // 读取按住持续时间（默认值：100ms）
    m_PressTime = IniHelper::getString("AUTOFIRE", "presstime", "100", CONFIG_PATH);
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
    
    // 使用全局变量（参考ovl-FtpAutoBack的g_timeoutItem）
    g_FireIntervalItem = new tsl::elm::ListItem("松开时间", m_FireInterval + "ms");
    g_FireIntervalItem->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            std::string FireInterval = IniHelper::getString("AUTOFIRE", "fireinterval", "100", CONFIG_PATH);
            tsl::changeTo<TimeSetting>(FireInterval, "fireinterval", "松开时间", true);
            return true;
        }
        return false;
    });
    list->addItem(g_FireIntervalItem);
    
    // 使用全局变量（参考ovl-FtpAutoBack的g_backupCountItem）
    g_PressTimeItem = new tsl::elm::ListItem("按住时间", m_PressTime + "ms");
    g_PressTimeItem->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            std::string PressTime = IniHelper::getString("AUTOFIRE", "presstime", "100", CONFIG_PATH);
            tsl::changeTo<TimeSetting>(PressTime, "presstime", "按住时间", true);
            return true;
        }
        return false;
    });
    list->addItem(g_PressTimeItem);
    
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

// ========== 时间设置界面 ==========

TimeSetting::TimeSetting(std::string currentValue, std::string configKey, 
                         std::string title, bool isGlobal)
    : m_currentTime(std::stoi(currentValue))
    , m_configKey(configKey)
    , m_title(title)
    , m_isGlobal(isGlobal)
    , m_list(nullptr)
    , m_needsRefocus(true)
    , m_frameCounter(0)
{
}

tsl::elm::Element* TimeSetting::createUI()
{
    std::string subtitle = m_isGlobal ? "全局配置" : "游戏独立配置";
    auto rootFrame = new tsl::elm::OverlayFrame(m_title, subtitle);
    
    m_list = new tsl::elm::List();
    m_list->addItem(new tsl::elm::CategoryHeader("单位：毫秒 (ms)"));
    
    // 生成 100-300ms，间隔20ms 的选项（共11个）
    for (int idx = 0; idx < 11; idx++) {
        int value = 100 + idx * 20;
        std::string valueStr = std::to_string(value);
        
        auto item = new tsl::elm::ListItem(valueStr + "ms");
        
        // 当前值显示勾选标记，并将值转换为索引
        if (value == m_currentTime) {
            item->setValue("\uE14B");
            m_currentTime = idx;
        }
        
        item->setClickListener([this, valueStr](u64 keys) {
            if (keys & HidNpadButton_A) {
                IniHelper::setString("AUTOFIRE", m_configKey, valueStr, CONFIG_PATH);
                
                // 更新全局 ListItem 显示值
                if (m_configKey == "fireinterval" && g_FireIntervalItem != nullptr) {
                    g_FireIntervalItem->setValue(valueStr + "ms");
                } else if (m_configKey == "presstime" && g_PressTimeItem != nullptr) {
                    g_PressTimeItem->setValue(valueStr + "ms");
                }
                
                tsl::goBack();
                return true;
            }
            return false;
        });
        
        m_list->addItem(item);
    }
    
    rootFrame->setContent(m_list);
    return rootFrame;
}

void TimeSetting::update()
{
    // 延迟2帧后设置焦点，确保 Tesla 框架初始化完成
    if (m_needsRefocus && m_list != nullptr) {
        m_frameCounter++;
        if (m_frameCounter >= 2) {
            // +1 是因为 CategoryHeader 占用了 list[0]
            m_list->setFocusedIndex(m_currentTime + 1);
            auto targetItem = m_list->getItemAtIndex(m_currentTime + 1);
            if (targetItem != nullptr) {
                this->requestFocus(targetItem, tsl::FocusDirection::None, false);
            }
            m_needsRefocus = false;
        }
    }
}

// ========== 总设置界面 ==========

// 构造函数
AutoKeySetting::AutoKeySetting()
{
    // 读取日志开关配置（默认值：false）
    m_logEnabled = IniHelper::getBool("LOG", "log", false, CONFIG_PATH);
    
    // 读取自动开启连发配置（默认值：false）
    m_autoEnabled = IniHelper::getBool("AUTOFIRE", "autoenable", false, CONFIG_PATH);
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
    listItemModule->setClickListener([listItemModule](u64 keys) {
        if (keys & HidNpadButton_A) {
            if (SysModuleManager::isRunning()) {
                // 关闭系统模块
                Result rc = SysModuleManager::stopModule();
                if (R_SUCCEEDED(rc)) {
                    listItemModule->setValue("关");
                }
            } else {
                // 启动系统模块
                Result rc = SysModuleManager::startModule();
                if (R_SUCCEEDED(rc)) {
                    listItemModule->setValue("开");
                }
            }
            return true;
        }
        return false;
    });
    list->addItem(listItemModule);

    // 添加分类标题
    auto categoryHeader2 = new tsl::elm::CategoryHeader(" 基础功能设置");
    list->addItem(categoryHeader2);
    
    // 添加默认连发
    auto listItemDefaultAuto = new tsl::elm::ListItem("默认连发", m_autoEnabled ? "开" : "关");
    listItemDefaultAuto->setClickListener([listItemDefaultAuto](u64 keys) {
        if (keys & HidNpadButton_A) {
            bool autoEnabled = IniHelper::getBool("AUTOFIRE", "autoenable", false, CONFIG_PATH);
            IniHelper::setBool("AUTOFIRE", "autoenable", !autoEnabled, CONFIG_PATH);
            listItemDefaultAuto->setValue(!autoEnabled ? "开" : "关");
            return true;
        }
        return false;
    });
    list->addItem(listItemDefaultAuto);
    
    // 添加调试日志
    auto listItemLogEnable = new tsl::elm::ListItem("调试日志", m_logEnabled ? "开" : "关");
    listItemLogEnable->setClickListener([listItemLogEnable](u64 keys) {
        if (keys & HidNpadButton_A) {
            bool logEnabled = IniHelper::getBool("LOG", "log", false, CONFIG_PATH);
            IniHelper::setBool("LOG", "log", !logEnabled, CONFIG_PATH);
            listItemLogEnable->setValue(!logEnabled ? "开" : "关");
            return true;
        }
        return false;
    });
    list->addItem(listItemLogEnable);

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
