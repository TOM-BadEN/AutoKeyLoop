#include "setting_turbo_config.hpp"
#include "game_monitor.hpp"
#include "ini_helper.hpp"
#include "ipc_manager.hpp"

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

namespace {
    // 储存当前选中的连发按键的位掩码
    u64 s_TurboButtons = 0;  
}

SettingTurboConfig::SettingTurboConfig(bool isGlobal, u64 currentTitleId)  
    : m_isGlobal(isGlobal)
{
    if (!m_isGlobal) {
        // 获取当前游戏名称
        GameMonitor::getTitleIdGameName(currentTitleId, m_gameName);
        // 生成当前游戏配置文件路径 示例：/config/KeyX/GameConfig/01007EF00011E000.ini
        snprintf(m_ConfigPath, sizeof(m_ConfigPath), "/config/KeyX/GameConfig/%016lX.ini", currentTitleId);
    }
    else {
        // 生成全局配置文件路径 示例：/config/KeyX/config.ini
        snprintf(m_ConfigPath, sizeof(m_ConfigPath), "/config/KeyX/config.ini");
    }

    // 读取速度配置（0=普通，1=高速）
    m_TurboSpeed = (IniHelper::getInt("AUTOFIRE", "presstime", 100, m_ConfigPath) == 100);
    // 读取连发按键配置（默认值：0 = 未设置连发）
    s_TurboButtons = static_cast<u64>(IniHelper::getInt("AUTOFIRE", "buttons", 0, m_ConfigPath));
}

tsl::elm::Element* SettingTurboConfig::createUI() {
    const char* title = m_isGlobal ? "全局配置" : "独立配置";
    const char* subtitle = m_isGlobal ? "设置全局默认连发参数" : m_gameName;
    
    auto frame = new tsl::elm::OverlayFrame(title, subtitle);
    auto list = new tsl::elm::List();
    
    auto listItemTurboKeySetting = new tsl::elm::CategoryHeader(" 连发参数配置");
    list->addItem(listItemTurboKeySetting);

    auto listItemTurboKey = new tsl::elm::ListItem("连发按键", ">");
    listItemTurboKey->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<SettingTurboButton>(m_ConfigPath, m_isGlobal);
            return true;
        }
        return false;
    });
    list->addItem(listItemTurboKey);

    auto listItemTurboHighSpeed = new tsl::elm::ListItem("高速连发", m_TurboSpeed ? "\uE14B" : "");
    auto listItemTurboLowSpeed = new tsl::elm::ListItem("普通连发", !m_TurboSpeed ? "\uE14B" : "");

    listItemTurboHighSpeed->setClickListener([listItemTurboHighSpeed, listItemTurboLowSpeed, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_TurboSpeed = true;
            IniHelper::setInt("AUTOFIRE", "presstime", 100, m_ConfigPath);
            IniHelper::setInt("AUTOFIRE", "fireinterval", 100, m_ConfigPath);
            g_ipcManager.sendReloadAutoFireCommand();
            listItemTurboHighSpeed->setValue("\uE14B");
            listItemTurboLowSpeed->setValue("");
            return true;
        }
        return false;
    });

    listItemTurboLowSpeed->setClickListener([listItemTurboHighSpeed, listItemTurboLowSpeed, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_TurboSpeed = false;
            IniHelper::setInt("AUTOFIRE", "presstime", 200, m_ConfigPath);
            IniHelper::setInt("AUTOFIRE", "fireinterval", 50, m_ConfigPath);
            g_ipcManager.sendReloadAutoFireCommand();
            listItemTurboHighSpeed->setValue("");
            listItemTurboLowSpeed->setValue("\uE14B");
            return true;
        }
        return false;
    });
    
    list->addItem(listItemTurboHighSpeed);
    list->addItem(listItemTurboLowSpeed);

    // 按键显示区域高度（720 - 标题97 - 底部73 - 分类标题63 - 3个列表项210 = 277）
    s32 buttonDisplayHeight = 277;
    // 添加按键布局显示区域（根据 s_TurboButtons 动态显示颜色）
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
        
        // === 根据 s_TurboButtons 显示按键颜色 ===
        // 行1: ZL, ZR
        s32 row1Y = baseY;
        r->drawString(ButtonIcon::ZL, false, lColumnX, row1Y, buttonSize, 
            a((s_TurboButtons & BTN_ZL) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::ZR, false, rColumnX, row1Y, buttonSize, 
            a((s_TurboButtons & BTN_ZR) ? lightBlueColor : whiteColor));
        
        // 行2: L, R
        s32 row2Y = baseY + buttonSize + rowSpacing;
        r->drawString(ButtonIcon::L, false, lColumnX, row2Y + 10, buttonSize, 
            a((s_TurboButtons & BTN_L) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::R, false, rColumnX, row2Y + 10, buttonSize, 
            a((s_TurboButtons & BTN_R) ? lightBlueColor : whiteColor));
        
        // 行3: 方向键上, X
        s32 row3Y = baseY + 2 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Up, false, dpadCenterX, row3Y, buttonSize, 
            a((s_TurboButtons & BTN_UP) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::X, false, rightColumnX, row3Y, buttonSize, 
            a((s_TurboButtons & BTN_X) ? lightBlueColor : whiteColor));
        
        // 行4: 方向键左右, Y/A
        s32 row4Y = baseY + 3 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Left, false, dpadLeftX, row4Y, buttonSize, 
            a((s_TurboButtons & BTN_LEFT) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::Right, false, dpadRightX, row4Y, buttonSize, 
            a((s_TurboButtons & BTN_RIGHT) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::Y, false, yButtonX, row4Y, buttonSize, 
            a((s_TurboButtons & BTN_Y) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::A, false, aButtonX, row4Y, buttonSize, 
            a((s_TurboButtons & BTN_A) ? lightBlueColor : whiteColor));
        
        // 行5: 方向键下, B
        s32 row5Y = baseY + 4 * (buttonSize + rowSpacing);
        r->drawString(ButtonIcon::Down, false, dpadCenterX, row5Y, buttonSize, 
            a((s_TurboButtons & BTN_DOWN) ? lightBlueColor : whiteColor));
        r->drawString(ButtonIcon::B, false, rightColumnX, row5Y, buttonSize, 
            a((s_TurboButtons & BTN_B) ? lightBlueColor : whiteColor));
    });
    
    list->addItem(buttonDisplay, buttonDisplayHeight);
    
    frame->setContent(list);
    return frame;
}


SettingTurboButton::SettingTurboButton(const char* configPath, bool isGlobal)
    : m_configPath(configPath)
    , m_isGlobal(isGlobal)
{
}

tsl::elm::Element* SettingTurboButton::createUI() {
    const char* subtitle = m_isGlobal ? "全局配置" : "独立配置";
    
    // 使用 HeaderOverlayFrame 以支持自定义头部
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    
    // 添加自定义头部（包含标题、副标题和底部右键提示）
    frame->setHeader(new tsl::elm::CustomDrawer([subtitle](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("连发按键", false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString(subtitle, false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
        renderer->drawString("\uE0EE  重置", false, 280, 693, 23, renderer->a(tsl::style::color::ColorText));
    }));
    
    auto list = new tsl::elm::List();
    list->addItem(new tsl::elm::CategoryHeader("选择连发按键（可多选）"));
    
    // 定义按键配置：名称、Unicode符号、位标志
    struct ButtonConfig {
        const char* name;
        const char* unicode;
        u64 flag;
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
        bool isSelected = (s_TurboButtons & btn.flag) != 0;
        
        // 拼接按键名称和图标
        std::string buttonName = ult::i18n(btn.name) + "  " + btn.unicode;
        auto item = new tsl::elm::ToggleListItem(buttonName, isSelected);
        
        item->setStateChangedListener([this, btn](bool state) {
            if (state) {
                s_TurboButtons |= btn.flag;  // 添加按键
            } else {
                s_TurboButtons &= ~btn.flag; // 移除按键
            }
            
            // 保存到配置文件
            IniHelper::setInt("AUTOFIRE", "buttons", s_TurboButtons, m_configPath);
            g_ipcManager.sendReloadAutoFireCommand();
        });
        
        list->addItem(item);
    }
    
    frame->setContent(list);
    return frame;
}

bool SettingTurboButton::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    
    // 监控右键，执行重置功能
    if (keysDown & HidNpadButton_Right) {
        s_TurboButtons = 0;
        IniHelper::setInt("AUTOFIRE", "buttons", s_TurboButtons, m_configPath);
        g_ipcManager.sendReloadAutoFireCommand();
        tsl::goBack();
        tsl::changeTo<SettingTurboButton>(m_configPath, m_isGlobal);
        return true;
    }
    
    return false;
}

