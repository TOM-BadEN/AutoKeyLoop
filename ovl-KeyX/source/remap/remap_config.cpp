#include "remap_config.hpp"
#include "game.hpp"
#include "ini_helper.hpp"
#include "ipc.hpp"

// Switch 按键 Unicode 图标
namespace ButtonIcon {
    constexpr const char* A      = "\uE0A0";  // A 按键
    constexpr const char* B      = "\uE0A1";  // B 按键
    constexpr const char* X      = "\uE0A2";  // X 按键
    constexpr const char* Y      = "\uE0A3";  // Y 按键
    constexpr const char* Up     = "\uE0AF";  // 方向键上
    constexpr const char* Down   = "\uE0B0";  // 方向键下
    constexpr const char* Left   = "\uE0B1";  // 方向键左
    constexpr const char* Right  = "\uE0B2";  // 方向键右
    constexpr const char* L      = "\uE0A4";  // L 按键
    constexpr const char* R      = "\uE0A5";  // R 按键
    constexpr const char* ZL     = "\uE0A6";  // ZL 按键
    constexpr const char* ZR     = "\uE0A7";  // ZR 按键
    constexpr const char* StickL = "\uE0C4";  // 左摇杆按下
    constexpr const char* StickR = "\uE0C5";  // 右摇杆按下
    constexpr const char* Start  = "\uE0B5";  // Start/Plus
    constexpr const char* Select = "\uE0B6";  // Select/Minus
}

namespace {
    struct ButtonMapping {
        const char* source;   // 源按键（固定）
        char target[8];       // 目标按键（可变）
    };
    
    ButtonMapping s_ButtonMappings[] = {
        {"A", "A"},
        {"B", "B"},
        {"X", "X"},
        {"Y", "Y"},
        {"Up", "Up"},
        {"Down", "Down"},
        {"Left", "Left"},
        {"Right", "Right"},
        {"L", "L"},
        {"R", "R"},
        {"ZL", "ZL"},
        {"ZR", "ZR"},
        {"StickL", "StickL"},
        {"StickR", "StickR"},
        {"Start", "Start"},
        {"Select", "Select"}
    };
    
    constexpr int BUTTON_COUNT = 16;
    
    // 当前配置上下文（全局变量）
    bool s_isGlobal = true;
    u64 s_titleId = 0;
    char s_configPath[256] = "";
    
    // 根据按键名称获取图标
    const char* getButtonIcon(const char* buttonName) {
        if (strcmp(buttonName, "A") == 0) return ButtonIcon::A;
        if (strcmp(buttonName, "B") == 0) return ButtonIcon::B;
        if (strcmp(buttonName, "X") == 0) return ButtonIcon::X;
        if (strcmp(buttonName, "Y") == 0) return ButtonIcon::Y;
        if (strcmp(buttonName, "Up") == 0) return ButtonIcon::Up;
        if (strcmp(buttonName, "Down") == 0) return ButtonIcon::Down;
        if (strcmp(buttonName, "Left") == 0) return ButtonIcon::Left;
        if (strcmp(buttonName, "Right") == 0) return ButtonIcon::Right;
        if (strcmp(buttonName, "L") == 0) return ButtonIcon::L;
        if (strcmp(buttonName, "R") == 0) return ButtonIcon::R;
        if (strcmp(buttonName, "ZL") == 0) return ButtonIcon::ZL;
        if (strcmp(buttonName, "ZR") == 0) return ButtonIcon::ZR;
        if (strcmp(buttonName, "StickL") == 0) return ButtonIcon::StickL;
        if (strcmp(buttonName, "StickR") == 0) return ButtonIcon::StickR;
        if (strcmp(buttonName, "Start") == 0) return ButtonIcon::Start;
        if (strcmp(buttonName, "Select") == 0) return ButtonIcon::Select;
        return "\uE142";
    }
}

SettingRemapConfig::SettingRemapConfig(bool isGlobal, u64 currentTitleId)
{
    // 设置全局配置上下文
    s_isGlobal = isGlobal;
    s_titleId = currentTitleId;
    
    if (!s_isGlobal) {
        // 获取当前游戏名称
        GameMonitor::getTitleIdGameName(currentTitleId, m_gameName);
        // 生成当前游戏配置文件路径
        snprintf(s_configPath, sizeof(s_configPath), "/config/KeyX/GameConfig/%016lX.ini", currentTitleId);
    }
    else {
        // 生成全局配置文件路径
        snprintf(s_configPath, sizeof(s_configPath), "/config/KeyX/config.ini");
    }
    
    // 读取映射配置
    loadMappings();
}

void SettingRemapConfig::loadMappings() {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        std::string temp = IniHelper::getString(
            "MAPPING",
            s_ButtonMappings[i].source,
            s_ButtonMappings[i].source,
            s_configPath
        );
        strncpy(s_ButtonMappings[i].target, temp.c_str(), 7);
        s_ButtonMappings[i].target[7] = '\0';
    }
}

tsl::elm::Element* SettingRemapConfig::createUI() {
    const char* title = s_isGlobal ? "全局配置" : "独立配置";
    const char* subtitle = s_isGlobal ? "设置全局映射按键" : m_gameName;
    
    // 使用 HeaderOverlayFrame
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    
    // 添加自定义头部（包含标题、副标题和底部提示）
    frame->setHeader(new tsl::elm::CustomDrawer([title, subtitle](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        // 绘制标题
        renderer->drawString(title, false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        // 绘制副标题
        renderer->drawString(subtitle, false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
        
        // 绘制底部提示
        renderer->drawString("\uE0EE  查看", false, 280, 693, 23, renderer->a(tsl::style::color::ColorText));
    }));
    
    auto list = new tsl::elm::List();
    
    // 添加分类标题
    list->addItem(new tsl::elm::CategoryHeader(" 当前映射列表"));
    
    // 遍历所有映射，创建列表项
    for (int i = 0; i < BUTTON_COUNT; i++) {
        const char* sourceIcon = getButtonIcon(s_ButtonMappings[i].source);
        const char* targetIcon = getButtonIcon(s_ButtonMappings[i].target);
        
        // 判断是否映射
        bool isMapped = (strcmp(s_ButtonMappings[i].source, s_ButtonMappings[i].target) != 0);
        
        // 设置右侧文本颜色
        tsl::Color valueColor = isMapped 
            ? tsl::Color{0x00, 0xDD, 0xFF, 0xFF}  // 绿色：已映射
            : tsl::style::color::ColorText;        // 默认颜色：未映射

        auto item = new tsl::elm::ListItemV2(
            ult::i18n("按键  ") + sourceIcon + "          \uE14A\uE14A",  // 左边：按键名称
            ult::i18n("按键  ") + targetIcon,   // 右边：目标按键图标
            valueColor                             // 右侧文本颜色
        );
        
        // 添加点击事件，跳转到编辑界面
        item->setClickListener([i](u64 keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<SettingRemapEdit>(i);
                return true;
            }
            return false;
        });
        
        list->addItem(item);
    }
    
    frame->setContent(list);
    return frame;
}

bool SettingRemapConfig::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_Right) {
        tsl::changeTo<SettingRemapDisplay>();
        return true;
    }
    return false;
}

SettingRemapDisplay::SettingRemapDisplay()
{
}

tsl::elm::Element* SettingRemapDisplay::createUI() {
    auto frame = new tsl::elm::OverlayFrame("映射布局", s_isGlobal ? "全局配置" : "独立配置");
    auto list = new tsl::elm::List();
    
    // 计算按键显示区域高度
    s32 buttonDisplayHeight = 450;
    
    // 添加按键布局显示区域（显示映射关系）
    auto buttonDisplay = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};      // 白色：未映射
        tsl::Color blueColor = {0x00, 0xDD, 0xFF, 0xFF};      // 蓝色：已映射
        
        const s32 buttonSize = 30;   // 按钮大小
        const s32 rowSpacing = 10;   // 行间距
        
        // === 水平位置计算 ===
        // 左侧：方向键组
        s32 dpadLeftX = x + 30;      // 方向键左
        s32 dpadCenterX = x + 70;    // 方向键上/下
        s32 dpadRightX = x + 110;    // 方向键右
        
        // L/ZL列：在方向键右的右边
        s32 lColumnX = dpadRightX + 35;
        
        // 右侧：ABXY按键组
        s32 aButtonX = x + w - 50;
        s32 rightColumnX = x + w - 90;
        s32 yButtonX = x + w - 130;
        
        // R/ZR列：在Y左边
        s32 rColumnX = yButtonX - 35;
        
        // === 垂直位置计算（7行布局）===
        const s32 layoutTotalHeight = 7 * buttonSize + 6 * rowSpacing;
        s32 baseY = y + (h - layoutTotalHeight) / 2 + buttonSize;
        
        // 辅助函数：绘制按键（根据映射关系选择颜色）
        auto drawButton = [&](const char* sourceName, s32 posX, s32 posY) {
            // 查找映射
            const char* targetIcon = nullptr;
            bool isMapped = false;
            
            for (int i = 0; i < BUTTON_COUNT; i++) {
                if (strcmp(s_ButtonMappings[i].source, sourceName) == 0) {
                    targetIcon = getButtonIcon(s_ButtonMappings[i].target);
                    isMapped = (strcmp(s_ButtonMappings[i].source, s_ButtonMappings[i].target) != 0);
                    break;
                }
            }
            
            if (targetIcon) {
                tsl::Color color = isMapped ? blueColor : whiteColor;
                r->drawString(targetIcon, false, posX, posY, buttonSize, a(color));
            }
        };
        
        // === 绘制按键布局 ===
        // 行1: ZL, ZR
        s32 row1Y = baseY;
        drawButton("ZL", lColumnX, row1Y);
        drawButton("ZR", rColumnX, row1Y);
        
        // 行2: L, R
        s32 row2Y = baseY + buttonSize + rowSpacing;
        drawButton("L", lColumnX, row2Y + 10);
        drawButton("R", rColumnX, row2Y + 10);
        
        // Select(-) 和 Start(+)：Y坐标在 ZR 和 R 之间
        s32 selectStartY = (row1Y + (row2Y + 10)) / 2;
        drawButton("Select", dpadLeftX, selectStartY);
        drawButton("Start", aButtonX, selectStartY);
        
        // 行3: 方向键上, X
        s32 row3Y = baseY + 2 * (buttonSize + rowSpacing);
        drawButton("Up", dpadCenterX, row3Y);
        drawButton("X", rightColumnX, row3Y);
        
        // 行4: 方向键左右, Y/A
        s32 row4Y = baseY + 3 * (buttonSize + rowSpacing);
        drawButton("Left", dpadLeftX, row4Y);
        drawButton("Right", dpadRightX, row4Y);
        drawButton("Y", yButtonX, row4Y);
        drawButton("A", aButtonX, row4Y);
        
        // 行5: 方向键下, B
        s32 row5Y = baseY + 4 * (buttonSize + rowSpacing);
        drawButton("Down", dpadCenterX, row5Y);
        drawButton("B", rightColumnX, row5Y);
        
        // 行6: StickL, StickR
        s32 row6Y = baseY + 5 * (buttonSize + rowSpacing);
        drawButton("StickL", lColumnX, row6Y);
        drawButton("StickR", rColumnX, row6Y);
    });
    
    list->addItem(buttonDisplay, buttonDisplayHeight);
    
    // 添加重置映射列表项
    auto resetItem = new tsl::elm::ListItem("重置映射", "按 \uE0F1 重置");
    list->addItem(resetItem);
    
    frame->setContent(list);
    return frame;
}

bool SettingRemapDisplay::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_Left) {
        tsl::goBack();
        return true;
    }

    if (keysDown & HidNpadButton_Plus) {
        // 重置映射
        resetMappings();
        g_ipcManager.sendReloadMappingCommand();
        // 刷新界面
        tsl::goBack();
        tsl::goBack();
        tsl::changeTo<SettingRemapConfig>(s_isGlobal, s_titleId);
        return true;
    }

    return false;
}

void SettingRemapDisplay::resetMappings() {
    // 保存到配置文件（写入默认值）
    for (int i = 0; i < BUTTON_COUNT; i++) {
        IniHelper::setString("MAPPING", 
            s_ButtonMappings[i].source, 
            s_ButtonMappings[i].source,
            s_configPath);
    }
}


SettingRemapEdit::SettingRemapEdit(int buttonIndex)
    : m_buttonIndex(buttonIndex)
{
}

tsl::elm::Element* SettingRemapEdit::createUI() {
    const char* sourceName = s_ButtonMappings[m_buttonIndex].source;
    const char* sourceIcon = getButtonIcon(sourceName);
    
    auto frame = new tsl::elm::OverlayFrame(
        "修改映射", 
        ult::i18n("选择 ") + sourceIcon + ult::i18n(" 的目标按键")
    );
    
    auto list = new tsl::elm::List();
    
    // 分类标题
    auto header = new tsl::elm::CategoryHeader("目标按键");
    list->addItem(header);
    
    // 显示所有 16 个可选按键
    for (int i = 0; i < BUTTON_COUNT; i++) {
        const char* targetName = s_ButtonMappings[i].source;  // 使用源按键名作为目标选项
        const char* targetIcon = getButtonIcon(targetName);
        
        // 判断是否是当前映射
        bool isCurrent = (strcmp(s_ButtonMappings[m_buttonIndex].target, targetName) == 0);
        
        auto item = new tsl::elm::ListItem(
            isCurrent ? "当前" : "",
            ult::i18n("按键  ") + targetIcon
        );
        
        item->setClickListener([this, targetName](u64 keys) {
            if (keys & HidNpadButton_A) {
                // 保存到 INI 文件
                IniHelper::setString("MAPPING", 
                    s_ButtonMappings[m_buttonIndex].source, 
                    targetName, 
                    s_configPath);
                g_ipcManager.sendReloadMappingCommand();
                // 设置刷新标志（实际刷新在 handleInput 中进行）
                m_needRefresh = true;
                return true;
            }
            return false;
        });
        
        list->addItem(item);
    }
    
    // 跳转到当前选中的项
    list->jumpToItem(ult::i18n("当前"), "");
    
    frame->setContent(list);
    return frame;
}

bool SettingRemapEdit::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    
    if (m_needRefresh) {
        tsl::goBack();
        tsl::goBack();
        tsl::changeTo<SettingRemapConfig>(s_isGlobal, s_titleId);
        return true;
    }
    
    return false;
}

