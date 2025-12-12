#include "turbo_config.hpp"
#include "game.hpp"
#include "ini_helper.hpp"
#include "ipc.hpp"
#include "hiddata.hpp"
#include "refresh.hpp"

namespace {
    u64 s_TurboButtons = 0;
    
    struct SpeedConfig {
        const char* name;
        int press;
        int release;
        tsl::Color color;
    };
    constexpr SpeedConfig SPEED_CONFIGS[] = {
        {"极速", 50, 50, {0xF, 0x5, 0x5, 0xF}},       // 红色
        {"高速", 100, 100, {0x00, 0xDD, 0xFF, 0xFF}}, // 蓝色
        {"普通", 200, 50, {0x00, 0xFF, 0xDD, 0xFF}},  // 标准颜色(00FFDD)
    };
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

    // 读取速度配置（0=极速, 1=高速, 2=普通）
    int press = IniHelper::getInt("AUTOFIRE", "presstime", 50, m_ConfigPath);
    if (press == 50) m_TurboSpeed = 0;
    else if (press == 100) m_TurboSpeed = 1;
    else m_TurboSpeed = 2;
    // 读取防止误触配置（0=关闭，1=开启）
    m_DelayStart = IniHelper::getInt("AUTOFIRE", "delaystart", 1, m_ConfigPath);
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


    auto& cfg = SPEED_CONFIGS[m_TurboSpeed];
    auto listItemTurboSpeed = new tsl::elm::ListItem("连发速度", cfg.name);
    listItemTurboSpeed->setValueColor(cfg.color);
    listItemTurboSpeed->setClickListener([listItemTurboSpeed, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_TurboSpeed = (m_TurboSpeed + 1) % 3;
            auto& newCfg = SPEED_CONFIGS[m_TurboSpeed];
            IniHelper::setInt("AUTOFIRE", "presstime", newCfg.press, m_ConfigPath);
            IniHelper::setInt("AUTOFIRE", "fireinterval", newCfg.release, m_ConfigPath);
            g_ipcManager.sendReloadAutoFireCommand();
            listItemTurboSpeed->setValue(newCfg.name);
            listItemTurboSpeed->setValueColor(newCfg.color);
            return true;
        }
        return false;
    });
    list->addItem(listItemTurboSpeed);

    list->addItem(new tsl::elm::CategoryHeader(" 延迟启动连发功能避免误触"));

    auto listItemDelayStart = new tsl::elm::ListItem("防止误触", m_DelayStart ? "开" : "关");
    listItemDelayStart->setClickListener([listItemDelayStart, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_DelayStart = !m_DelayStart;
            IniHelper::setInt("AUTOFIRE", "delaystart", m_DelayStart ? 1 : 0, m_ConfigPath);
            g_ipcManager.sendReloadAutoFireCommand();
            listItemDelayStart->setValue(m_DelayStart ? "开" : "关");
            return true;
        }
        return false;
    });
    list->addItem(listItemDelayStart);
    

    // 按键显示区域高度（720 - 标题97 - 底部73 - 分类标题63 - 3个列表项210 = 277）
    s32 buttonDisplayHeight = 225;
    // 添加按键布局显示区域（根据 s_TurboButtons 动态显示颜色）
    auto buttonDisplay = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};
        tsl::Color lightBlueColor = {0x00, 0xDD, 0xFF, 0xFF};  // 亮天蓝色
        
        const s32 buttonSize = 30 * 0.8;   // 按钮大小
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
    m_toggleItems.clear();
    for (const auto& btn : TurboConfig::Buttons) {
        bool isSelected = (s_TurboButtons & btn.flag) != 0;
        const char* icon = HidHelper::getIconByMask(btn.flag);
        std::string buttonName = std::string(ult::i18n(btn.name)) + "  " + icon;
        auto item = new tsl::elm::ToggleListItem(buttonName, isSelected);
        item->setStateChangedListener([this, btn](bool state) {
            Refresh::RefrRequest(Refresh::MainMenu);
            if (state) s_TurboButtons |= btn.flag;
            else s_TurboButtons &= ~btn.flag;
            IniHelper::setInt("AUTOFIRE", "buttons", s_TurboButtons, m_configPath);
            g_ipcManager.sendReloadAutoFireCommand();
        });
        m_toggleItems.push_back(item);
        list->addItem(item);
    }
    
    frame->setContent(list);
    return frame;
}

void SettingTurboButton::update() {
    if (Refresh::RefrConsume(Refresh::TurboButton)) {
        for (auto* item : m_toggleItems) item->setState(false);
    }
}

bool SettingTurboButton::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    
    // 监控右键，执行重置功能
    if (keysDown & HidNpadButton_Right) {
        s_TurboButtons = 0;
        IniHelper::setInt("AUTOFIRE", "buttons", s_TurboButtons, m_configPath);
        Refresh::RefrRequest(Refresh::TurboButton);
        g_ipcManager.sendReloadAutoFireCommand();
        return true;
    }
    
    return false;
}

