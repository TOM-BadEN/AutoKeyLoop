/**

写着写着发现不太对，应该整个设置模块是一个大类，下面分一堆小类
不过懒得改了，就这样吧


*/


#include "setting_manager.hpp"
#include "sysmodule_manager.hpp"
#include "ini_helper.hpp"
#include <string>
#include "ipc_manager.hpp"
#include "main_menu.hpp"  
#define CONFIG_PATH "/config/AutoKeyLoop/config.ini"

// Switch 按键 Unicode 图标
namespace ButtonIcon {
    constexpr const char* A      = "\uE0A0";  // A 按键
    constexpr const char* B      = "\uE0A1";  // B 按键
    constexpr const char* X      = "\uE0A2";  // X 按键
    constexpr const char* Y      = "\uE0A3";  // Y 按键
    constexpr const char* L      = "\uE0A4";  // L 按键
    constexpr const char* R      = "\uE0A5";  // R 按键
    constexpr const char* ZL     = "\uE0A6";  // ZL 按键
    constexpr const char* ZR     = "\uE0A7";  // ZR 按键
    constexpr const char* Up     = "\uE0AF";  // 方向键上
    constexpr const char* Down   = "\uE0B0";  // 方向键下
    constexpr const char* Left   = "\uE0B1";  // 方向键左
    constexpr const char* Right  = "\uE0B2";  // 方向键右
}

// 按键位标志定义（与 HidNpadButton 位索引保持一致）
enum ButtonFlags : u64 {
    BTN_A     = 1ULL << 0,   // HidNpadButton_A
    BTN_B     = 1ULL << 1,   // HidNpadButton_B
    BTN_X     = 1ULL << 2,   // HidNpadButton_X
    BTN_Y     = 1ULL << 3,   // HidNpadButton_Y
    BTN_L     = 1ULL << 6,   // HidNpadButton_L（跳过 StickL/StickR）
    BTN_R     = 1ULL << 7,   // HidNpadButton_R
    BTN_ZL    = 1ULL << 8,   // HidNpadButton_ZL
    BTN_ZR    = 1ULL << 9,   // HidNpadButton_ZR
    BTN_LEFT  = 1ULL << 12,  // HidNpadButton_Left（跳过 Plus/Minus）
    BTN_UP    = 1ULL << 13,  // HidNpadButton_Up
    BTN_RIGHT = 1ULL << 14,  // HidNpadButton_Right
    BTN_DOWN  = 1ULL << 15   // HidNpadButton_Down
};

// 文件作用域静态变量 - 只在本文件内的GUI类之间共享
static tsl::elm::ListItem* g_FireIntervalItem_Global = nullptr;
static tsl::elm::ListItem* g_PressTimeItem_Global = nullptr;
static tsl::elm::ListItem* g_FireIntervalItem_Game = nullptr;
static tsl::elm::ListItem* g_PressTimeItem_Game = nullptr;
static u64 g_selectedButtons_Global = 0;  // 全局配置用，储存当前选中的连发按键（位掩码，与HidNpadButton兼容）
static u64 g_selectedButtons_Game = 0;  // 独立配置用，储存当前选中的连发按键（位掩码，与HidNpadButton兼容）



// ========== 游戏设置界面 ==========

// 构造函数
GameSetting::GameSetting(std::string titleId, std::string gameName)
    : m_titleId(titleId)
    , m_gameName(gameName)
{

    // 生成当前游戏配置文件路径 示例：/config/AutoKeyLoop/GameConfig/01007EF00011E000.ini
    m_gameConfigPath = "/config/AutoKeyLoop/GameConfig/" + m_titleId + ".ini";

    // 读取连发间隔时间（默认值：100ms）
    m_FireInterval = IniHelper::getString("AUTOFIRE", "fireinterval", "100", m_gameConfigPath);
    
    // 读取按住持续时间（默认值：100ms）
    m_PressTime = IniHelper::getString("AUTOFIRE", "presstime", "100", m_gameConfigPath);
    
    // 读取连发按键配置（默认值：0 = 无按键）
    std::string buttonsStr = IniHelper::getString("AUTOFIRE", "buttons", "0", m_gameConfigPath);
    g_selectedButtons_Game = std::stoull(buttonsStr);  // 字符串转u64


}

// 创建用户界面
tsl::elm::Element* GameSetting::createUI()
{
    // 创建根框架
    auto rootFrame = new tsl::elm::OverlayFrame("独立配置", m_gameName);
    
    // 创建列表
    auto list = new tsl::elm::List();
    

    // 添加分类标题
    auto categoryHeader2 = new tsl::elm::CategoryHeader(" 连发参数设置");
    list->addItem(categoryHeader2);

    auto listItem4 = new tsl::elm::ListItem("连发按键", ">");
    listItem4->setClickListener([path = m_gameConfigPath](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<ButtonSetting>(g_selectedButtons_Game, path, false);
            return true;
        }
        return false;
    });
    list->addItem(listItem4);
    
    g_FireIntervalItem_Game = new tsl::elm::ListItem("松开时间", m_FireInterval + "ms");
    g_FireIntervalItem_Game->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<TimeSetting>(m_FireInterval, "fireinterval", "松开时间", m_gameConfigPath, false);
            return true;
        }
        return false;
    });
    list->addItem(g_FireIntervalItem_Game);
    
    g_PressTimeItem_Game = new tsl::elm::ListItem("按住时间", m_PressTime + "ms");
    g_PressTimeItem_Game->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<TimeSetting>(m_PressTime, "presstime", "按住时间", m_gameConfigPath, false);
            return true;
        }
        return false;
    });
    list->addItem(g_PressTimeItem_Game);

    // 按键显示区域高度（720 - 标题97 - 底部73 - 分类标题63 - 3个列表项210 = 277）
    s32 buttonDisplayHeight = 277;
    
    // 添加按键布局显示区域（根据 g_selectedButtons_Game 动态显示颜色）
    auto buttonDisplay = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};
        tsl::Color lightBlueColor = {0x00, 0xDD, 0xFF, 0xFF};  // 亮天蓝色
        
        const s32 buttonSize = 30;   // 按钮大小
        const s32 rowSpacing = 10;   // 行间距
        
        // === 水平位置计算 ===
        // 左侧：方向键组
        s32 dpadLeftX = x + 40;      // 方向键左
        s32 dpadCenterX = x + 80;    // 方向键上/下
        s32 dpadRightX = x + 120;    // 方向键右
        
        // L/ZL列：在方向键右的右边，保持间距
        s32 lColumnX = dpadRightX + 35;  // L和ZL垂直对齐
        
        // 右侧：ABXY按键组
        s32 aButtonX = x + w - 40;    // A按键
        s32 rightColumnX = x + w - 80;  // X和B按键（垂直对齐）
        s32 yButtonX = x + w - 120;   // Y按键
        
        // R/ZR列：在Y左边，保持间距
        s32 rColumnX = yButtonX - 35;  // R和ZR垂直对齐
        
        // === 垂直位置计算 ===
        // 总高度 = 5行×30px + 4个间距×10px = 190px
        const s32 layoutTotalHeight = 5 * buttonSize + 4 * rowSpacing;
        
        // 垂直居中（drawString的y参数是文字底部，需加buttonSize偏移）
        s32 baseY = y + (h - layoutTotalHeight) / 2 + buttonSize;
        
        // === 根据 g_selectedButtons_Game 显示按键颜色 ===
        // 行1: ZL, ZR
        s32 row1Y = baseY;
        r->drawString(ButtonIcon::ZL, false, lColumnX, row1Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_ZL) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::ZR, false, rColumnX, row1Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_ZR) ? lightBlueColor : whiteColor));
        
        // 行2: L, R
        s32 row2Y = baseY + buttonSize + rowSpacing;
        r->drawString(ButtonIcon::L, false, lColumnX, row2Y + 10, buttonSize, 
            a((g_selectedButtons_Game & BTN_L) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::R, false, rColumnX, row2Y + 10, buttonSize, 
            a((g_selectedButtons_Game & BTN_R) ? lightBlueColor : whiteColor));
        
        // 行3: 方向键上, X
        s32 row3Y = baseY + 2 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Up, false, dpadCenterX, row3Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_UP) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::X, false, rightColumnX, row3Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_X) ? lightBlueColor : whiteColor));
        
        // 行4: 方向键左右, Y/A
        s32 row4Y = baseY + 3 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Left, false, dpadLeftX, row4Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_LEFT) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::Right, false, dpadRightX, row4Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_RIGHT) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::Y, false, yButtonX, row4Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_Y) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::A, false, aButtonX, row4Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_A) ? lightBlueColor : whiteColor));
        
        // 行5: 方向键下, B
        s32 row5Y = baseY + 4 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Down, false, dpadCenterX, row5Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_DOWN) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::B, false, rightColumnX, row5Y, buttonSize, 
            a((g_selectedButtons_Game & BTN_B) ? lightBlueColor : whiteColor));
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
    
    // 读取连发按键配置（默认值：0 = 无按键）
    std::string buttonsStr = IniHelper::getString("AUTOFIRE", "buttons", "0", CONFIG_PATH);
    g_selectedButtons_Global = std::stoull(buttonsStr);  // 字符串转u64
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
    listItem4->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<ButtonSetting>(g_selectedButtons_Global, CONFIG_PATH, true);
            return true;
        }
        return false;
    });
    list->addItem(listItem4);
    
    // 使用全局变量（参考ovl-FtpAutoBack的g_timeoutItem）
    g_FireIntervalItem_Global = new tsl::elm::ListItem("松开时间", m_FireInterval + "ms");
    g_FireIntervalItem_Global->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            std::string FireInterval = IniHelper::getString("AUTOFIRE", "fireinterval", "100", CONFIG_PATH);
            tsl::changeTo<TimeSetting>(FireInterval, "fireinterval", "松开时间", CONFIG_PATH, true);
            return true;
        }
        return false;
    });
    list->addItem(g_FireIntervalItem_Global);
    
    // 使用全局变量（参考ovl-FtpAutoBack的g_backupCountItem）
    g_PressTimeItem_Global = new tsl::elm::ListItem("按住时间", m_PressTime + "ms");
    g_PressTimeItem_Global->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            std::string PressTime = IniHelper::getString("AUTOFIRE", "presstime", "100", CONFIG_PATH);
            tsl::changeTo<TimeSetting>(PressTime, "presstime", "按住时间", CONFIG_PATH, true);
            return true;
        }
        return false;
    });
    list->addItem(g_PressTimeItem_Global);
    
    // 按键显示区域高度（720 - 标题97 - 底部73 - 分类标题63 - 3个列表项210 = 277）
    s32 buttonDisplayHeight = 277;
    
    // 添加按键布局显示区域（根据 g_selectedButtons_Global 动态显示颜色）
    auto buttonDisplay = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};
        tsl::Color lightBlueColor = {0x00, 0xDD, 0xFF, 0xFF};  // 选中颜色
        
        const s32 buttonSize = 30;
        const s32 rowSpacing = 10;
        
        // === 水平位置计算 ===
        // 左侧：方向键组
        s32 dpadLeftX = x + 40;      // 方向键左
        s32 dpadCenterX = x + 80;    // 方向键上/下
        s32 dpadRightX = x + 120;    // 方向键右
        
        // L/ZL列：在方向键右的右边，保持间距
        s32 lColumnX = dpadRightX + 35;  // L和ZL垂直对齐
        
        // 右侧：ABXY按键组
        s32 aButtonX = x + w - 40;    // A按键
        s32 rightColumnX = x + w - 80;  // X和B按键（垂直对齐）
        s32 yButtonX = x + w - 120;   // Y按键
        
        // R/ZR列：在Y左边，保持间距
        s32 rColumnX = yButtonX - 35;  // R和ZR垂直对齐
        
        const s32 layoutTotalHeight = 5 * buttonSize + 4 * rowSpacing;
        s32 baseY = y + (h - layoutTotalHeight) / 2 + buttonSize;
        
        // === 根据 g_selectedButtons_Global 显示按键颜色 ===
        // 行1: ZL, ZR
        s32 row1Y = baseY;
        r->drawString(ButtonIcon::ZL, false, lColumnX, row1Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_ZL) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::ZR, false, rColumnX, row1Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_ZR) ? lightBlueColor : whiteColor));
        
        // 行2: L, R
        s32 row2Y = baseY + buttonSize + rowSpacing;
        r->drawString(ButtonIcon::L, false, lColumnX, row2Y + 10, buttonSize, 
            a((g_selectedButtons_Global & BTN_L) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::R, false, rColumnX, row2Y + 10, buttonSize, 
            a((g_selectedButtons_Global & BTN_R) ? lightBlueColor : whiteColor));
        
        // 行3: 方向键上, X
        s32 row3Y = baseY + 2 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Up, false, dpadCenterX, row3Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_UP) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::X, false, rightColumnX, row3Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_X) ? lightBlueColor : whiteColor));
        
        // 行4: 方向键左右, Y/A
        s32 row4Y = baseY + 3 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Left, false, dpadLeftX, row4Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_LEFT) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::Right, false, dpadRightX, row4Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_RIGHT) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::Y, false, yButtonX, row4Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_Y) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::A, false, aButtonX, row4Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_A) ? lightBlueColor : whiteColor));
        
        // 行5: 方向键下, B
        s32 row5Y = baseY + 4 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Down, false, dpadCenterX, row5Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_DOWN) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::B, false, rightColumnX, row5Y, buttonSize, 
            a((g_selectedButtons_Global & BTN_B) ? lightBlueColor : whiteColor));
    });
    list->addItem(buttonDisplay, buttonDisplayHeight);
    
    // 将列表添加到框架
    rootFrame->setContent(list);
    
    return rootFrame;
}

// ========== 按键设置界面 ==========

ButtonSetting::ButtonSetting(u64 currentButtons, std::string configPath, bool isGlobal)
    : m_selectedButtons(currentButtons)
    , m_configPath(configPath)
    , m_isGlobal(isGlobal)
{
}

tsl::elm::Element* ButtonSetting::createUI()
{
    std::string subtitle = m_isGlobal ? "全局配置" : "独立配置";
    auto rootFrame = new tsl::elm::OverlayFrame("连发按键", subtitle);
    
    auto list = new tsl::elm::List();
    list->addItem(new tsl::elm::CategoryHeader("选择连发按键（可多选）"));
    
    // 定义按键配置：名称、Unicode符号、位标志
    struct ButtonConfig {
        const char* name;
        const char* unicode;
        int flag;
    };
    
    ButtonConfig buttons[] = {
        {"按键", ButtonIcon::A, BTN_A},
        {"按键", ButtonIcon::B, BTN_B},
        {"按键", ButtonIcon::X, BTN_X},
        {"按键", ButtonIcon::Y, BTN_Y},
        {"按键", ButtonIcon::L, BTN_L},
        {"按键", ButtonIcon::R, BTN_R},
        {"按键", ButtonIcon::ZL, BTN_ZL},
        {"按键", ButtonIcon::ZR, BTN_ZR},
        {"方向键上", ButtonIcon::Up, BTN_UP},
        {"方向键下", ButtonIcon::Down, BTN_DOWN},
        {"方向键左", ButtonIcon::Left, BTN_LEFT},
        {"方向键右", ButtonIcon::Right, BTN_RIGHT}
    };
    
    for (const auto& btn : buttons) {
        bool isSelected = (m_selectedButtons & btn.flag) != 0;
        auto item = new tsl::elm::ToggleListItem(
            std::string(btn.name) + "  " + btn.unicode, 
            isSelected
        );
        
        item->setStateChangedListener([this, btn](bool state) {
            if (state) {
                m_selectedButtons |= btn.flag;  // 添加按键
            } else {
                m_selectedButtons &= ~btn.flag; // 移除按键
            }
            
            // 保存到配置文件（u64转为字符串保存，避免溢出）
            IniHelper::setString("AUTOFIRE", "buttons", std::to_string(m_selectedButtons), m_configPath);
            
            // 更新全局变量（用于CustomDrawer显示）
            if (m_isGlobal) g_selectedButtons_Global = m_selectedButtons;
            else g_selectedButtons_Game = m_selectedButtons;
            
            // 更新主菜单的按钮显示
            MainMenu::RefreshButtonsConfig();
            
            // 通知系统模块重新加载配置
            if (SysModuleManager::isRunning()) g_ipcManager.sendReloadConfigCommand();
        });
        
        list->addItem(item);
    }
    
    rootFrame->setContent(list);
    return rootFrame;
}

// ========== 时间设置界面 ==========

TimeSetting::TimeSetting(std::string currentValue, std::string configKey, 
                         std::string title, std::string configPath, bool isGlobal)
    : m_currentTime(std::stoi(currentValue))
    , m_configKey(configKey)
    , m_title(title)
    , m_configPath(configPath)
    , m_isGlobal(isGlobal)  // 是修改全局设置还是独立设置
    , m_list(nullptr)
    , m_needsRefocus(true)
    , m_frameCounter(0)
{
}

tsl::elm::Element* TimeSetting::createUI()
{
    std::string subtitle = m_isGlobal ? "全局配置" : "独立配置";
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
                IniHelper::setString("AUTOFIRE", m_configKey, valueStr, m_configPath);
                
                // 更新全局 ListItem 显示值
                if (m_configKey == "fireinterval" && g_FireIntervalItem_Global != nullptr) {
                    if (m_isGlobal) g_FireIntervalItem_Global->setValue(valueStr + "ms");
                    else g_FireIntervalItem_Game->setValue(valueStr + "ms");
                } else if (m_configKey == "presstime" && g_PressTimeItem_Global != nullptr) {
                    if (m_isGlobal) g_PressTimeItem_Global->setValue(valueStr + "ms");
                    else g_PressTimeItem_Game->setValue(valueStr + "ms");
                }
                if (SysModuleManager::isRunning()) g_ipcManager.sendReloadConfigCommand();
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
AutoKeySetting::AutoKeySetting(std::string titleId, std::string gameName)
    : m_titleId(titleId)
    , m_gameName(gameName)
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

    // 检查是否在游戏中
    bool isInGame = (m_titleId != "0000000000000000");

    auto listItemIndependentSetting = new tsl::elm::ListItemV2("独立配置", isInGame ? ">" : "不可用");
    
    // 如果不在游戏中，设置值为红色
    if (!isInGame) {
        tsl::Color redColor = {0xF, 0x5, 0x5, 0xF};  // 淡红色/粉红色（4位RGBA）
        listItemIndependentSetting->setValueColor(redColor);
    }
    
    listItemIndependentSetting->setClickListener([this, isInGame](u64 keys) {
        if (keys & HidNpadButton_A) {
            if (isInGame) tsl::changeTo<GameSetting>(m_titleId, m_gameName);
            return true;
        }
        return false;
    });
    list->addItem(listItemIndependentSetting);

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
            SysModuleManager::restartModule();
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
            SysModuleManager::restartModule();
            return true;
        }
        
        return false;
    });
    list->addItem(listItemLogEnable);

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
    
    // 将列表添加到框架
    rootFrame->setContent(list);
    
    return rootFrame;
}
