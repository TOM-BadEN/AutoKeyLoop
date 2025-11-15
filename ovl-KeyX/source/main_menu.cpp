#include "main_menu.hpp"
#include "game.hpp"
#include "ini_helper.hpp"
#include "ipc.hpp"
#include "sysmodule.hpp"
#include "main_menu_setting.hpp"
#include <ultra.hpp>
#include "hiddata.hpp"
#include "refresh.hpp"

// Tesla插件界面尺寸常量定义
#define TESLA_VIEW_HEIGHT 720      // Tesla插件总高度
#define TESLA_TITLE_HEIGHT 97      // 标题栏高度
#define TESLA_BOTTOM_HEIGHT 73     // 底部按钮栏高度
#define LIST_ITEM_HEIGHT 70        // 列表项高度
#define SPACING 20  // 间距常量

// 全局配置文件路径
constexpr const char* CONFIG_PATH = "/config/KeyX/config.ini";

// 按键映射初始化结构体
static MappingDef::ButtonMapping s_buttonMappings[MappingDef::BUTTON_COUNT] = {
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

void MainMenu::RefreshData() {
    
    /* 
     *   检查游戏TID
     *   目标是根据游戏TID，确认当前是否在游戏中
     *   如果不在游戏中，那配置文件使用全局配置
     *   如果在游戏中，那配置文件根据游戏配置文件中的设置来决定
     */
    u64 currentTitleId = GameMonitor::getCurrentTitleId();
    m_textAreaInfo.isInGame = (currentTitleId != 0) && SysModuleManager::isRunning();
    snprintf(m_textAreaInfo.gameId, sizeof(m_textAreaInfo.gameId), "%016lX", currentTitleId);
    m_textAreaInfo.GameConfigPath = "/config/KeyX/GameConfig/" + std::string(m_textAreaInfo.gameId) + ".ini";
    m_textAreaInfo.isGlobalConfig = IniHelper::getBool("AUTOFIRE", "globconfig", true, m_textAreaInfo.GameConfigPath);
    std::string SwitchConfigPath = (m_textAreaInfo.isInGame && ult::isFile(m_textAreaInfo.GameConfigPath)) ? m_textAreaInfo.GameConfigPath : CONFIG_PATH;
    m_textAreaInfo.isAutoFireEnabled = IniHelper::getBool("AUTOFIRE", "autoenable", false, SwitchConfigPath);
    m_textAreaInfo.isAutoRemapEnabled = IniHelper::getBool("MAPPING", "autoenable", false, SwitchConfigPath);
    m_textAreaInfo.isAutoMacroEnabled = IniHelper::getBool("MACRO", "autoenable", false, m_textAreaInfo.GameConfigPath);   // 宏只读独立游戏配置
    if (m_AutoFireEnableItem != nullptr) m_AutoFireEnableItem->setValue(m_textAreaInfo.isAutoFireEnabled ? "已开启" : "已关闭");
    if (m_AutoRemapEnableItem != nullptr) m_AutoRemapEnableItem->setValue(m_textAreaInfo.isAutoRemapEnabled ? "已开启" : "已关闭");
    if (m_AutoMacroEnableItem != nullptr) m_AutoMacroEnableItem->setValue(m_textAreaInfo.isAutoMacroEnabled ? "已开启" : "已关闭");

    // 读取按钮掩码值用于绘制按钮图标
    std::string ConfigPath = m_textAreaInfo.isGlobalConfig ? CONFIG_PATH : m_textAreaInfo.GameConfigPath;
    std::string buttonsStr = IniHelper::getString("AUTOFIRE", "buttons", "0", ConfigPath);
    m_textAreaInfo.buttons = std::stoull(buttonsStr);
    
    // 读取映射配置
    for (int i = 0; i < MappingDef::BUTTON_COUNT; i++) {
        std::string temp = IniHelper::getString("MAPPING", s_buttonMappings[i].source, s_buttonMappings[i].source, ConfigPath);
        strncpy(s_buttonMappings[i].target, temp.c_str(), 7);
        s_buttonMappings[i].target[7] = '\0';
    }

    // 读取所有宏的快捷键
    m_macroHotKey = 0;
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, m_textAreaInfo.GameConfigPath);
    for (int i = 1; i <= macroCount; i++) {
        u64 combo = static_cast<u64>(IniHelper::getInt("MACRO", "macro_combo_" + std::to_string(i), 0, m_textAreaInfo.GameConfigPath));
        m_macroHotKey |= combo;
    }
    m_macroHotKey &= ~ (ButtonMask::BTN_ZL | ButtonMask::BTN_ZR);
    
}


// 连发功能开关
void MainMenu::AutoKeyToggle() {
    if (!m_textAreaInfo.isInGame) return;
    // 根据状态发送IPC命令
    Result rc = m_textAreaInfo.isAutoFireEnabled ? g_ipcManager.sendDisableAutoFireCommand() : g_ipcManager.sendEnableAutoFireCommand();
    if (R_FAILED(rc)) return;
    // 切换状态，开变关-关变开
    m_textAreaInfo.isAutoFireEnabled = !m_textAreaInfo.isAutoFireEnabled;
    m_AutoFireEnableItem->setValue(m_textAreaInfo.isAutoFireEnabled ? "已开启" : "已关闭");
    IniHelper::setBool("AUTOFIRE", "globconfig", m_textAreaInfo.isGlobalConfig, m_textAreaInfo.GameConfigPath);
    IniHelper::setBool("AUTOFIRE", "autoenable", m_textAreaInfo.isAutoFireEnabled, m_textAreaInfo.GameConfigPath);
}

// 映射功能开关
void MainMenu::AutoRemapToggle() {
    if (!m_textAreaInfo.isInGame) return;
    // 根据状态发送IPC命令
    Result rc = m_textAreaInfo.isAutoRemapEnabled ? g_ipcManager.sendDisableMappingCommand() : g_ipcManager.sendEnableMappingCommand();
    if (R_FAILED(rc)) return;
    // 切换状态，开变关-关变开
    m_textAreaInfo.isAutoRemapEnabled = !m_textAreaInfo.isAutoRemapEnabled;
    m_AutoRemapEnableItem->setValue(m_textAreaInfo.isAutoRemapEnabled ? "已开启" : "已关闭");
    IniHelper::setBool("AUTOFIRE", "globconfig", m_textAreaInfo.isGlobalConfig, m_textAreaInfo.GameConfigPath);
    IniHelper::setBool("MAPPING", "autoenable", m_textAreaInfo.isAutoRemapEnabled, m_textAreaInfo.GameConfigPath);
}

// 宏功能开关
void MainMenu::AutoMacroToggle() {
    if (!m_textAreaInfo.isInGame) return;
    // 根据状态发送IPC命令
    Result rc = m_textAreaInfo.isAutoMacroEnabled ? g_ipcManager.sendDisableMacroCommand() : g_ipcManager.sendEnableMacroCommand();
    if (R_FAILED(rc)) return;
    // 切换状态，开变关-关变开
    m_textAreaInfo.isAutoMacroEnabled = !m_textAreaInfo.isAutoMacroEnabled;
    m_AutoMacroEnableItem->setValue(m_textAreaInfo.isAutoMacroEnabled ? "已开启" : "已关闭");
    IniHelper::setBool("MACRO", "autoenable", m_textAreaInfo.isAutoMacroEnabled, m_textAreaInfo.GameConfigPath);
}

// 配置切换（全局/独立）
void MainMenu::ConfigToggle() {
    if (!m_textAreaInfo.isInGame) return;
    m_textAreaInfo.isGlobalConfig = !m_textAreaInfo.isGlobalConfig;
    IniHelper::setBool("AUTOFIRE", "globconfig", m_textAreaInfo.isGlobalConfig, m_textAreaInfo.GameConfigPath);
    IniHelper::setBool("AUTOFIRE", "autoenable", m_textAreaInfo.isAutoFireEnabled, m_textAreaInfo.GameConfigPath);
    IniHelper::setBool("MAPPING", "autoenable", m_textAreaInfo.isAutoRemapEnabled, m_textAreaInfo.GameConfigPath);
    // 刷新界面显示
    RefreshData();
    // 发送重载配置命令
    Result rc = g_ipcManager.sendReloadBasicCommand();
    if (R_FAILED(rc)) return;
}

// 主菜单构造函数
MainMenu::MainMenu()
{
    // 刷新数据（初始化数据）
    RefreshData();
}

// 创建用户界面
tsl::elm::Element* MainMenu::createUI()
{  
    // 动态计算文本区域高度
    s32 TextAreaHeight = (TESLA_VIEW_HEIGHT - TESLA_TITLE_HEIGHT - TESLA_BOTTOM_HEIGHT) - (4 * LIST_ITEM_HEIGHT) - SPACING * 2;

    // 创建覆盖层框架，使用自定义头部（指定高度97避免滚动条）
    auto frame = new tsl::elm::HeaderOverlayFrame(TESLA_TITLE_HEIGHT);
    
    // 自定义头部绘制器
    frame->setHeader(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        // 左侧：标题和版本
        renderer->drawString("按键助手", false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString(APP_VERSION_STRING, false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
        
        // 右侧：两行信息（往左移，避免截断，屏幕可见宽度约615px）
        // 垂直居中：高度97，行间距25，第一行Y=36，第二行Y=61
        renderer->drawString("TID :", false, 235, 36, 15, renderer->a(tsl::style::color::ColorText));
        renderer->drawString(m_textAreaInfo.gameId, false, 275, 36, 15, renderer->a(tsl::style::color::ColorHighlight));
        
        renderer->drawString("配置:", false, 235, 61, 15, renderer->a(tsl::style::color::ColorText));
        const char* config = m_textAreaInfo.isGlobalConfig ? "全局配置" : "独立配置";
        renderer->drawString(config, false, 275, 61, 15, renderer->a(tsl::style::color::ColorHighlight));

        // 底部按钮（使用绝对坐标）
        renderer->drawString("\uE0EE  设置", false, 280, 693, 23, renderer->a(tsl::style::color::ColorText));

    }));

    // 创建主列表容器
    auto mainList = new tsl::elm::List();

    // ============= 上半部分：纯文本显示区域 =============
    // 创建自定义绘制器来显示纯文本内容（使用结构体成员数量）
    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        
        
        // 如果在游戏中，绘制按键图标
        if (m_textAreaInfo.isInGame) {
            tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};          // 白色：未映射
            tsl::Color blueColor = {0x00, 0xDD, 0xFF, 0xFF};           // 亮天蓝色：已映射
            tsl::Color yellowColor = {0xFF, 0xFF, 0x00, 0xFF};         // 黄色：连发小点
            tsl::Color redColor = {0xFF, 0x00, 0x00, 0xFF};            // 红色：宏小点
            
            const s32 buttonSize = 25;   // 按钮大小（保持不变）
            const s32 rowSpacing = 7;    // 行间距（8 × 0.9 ≈ 7）
            
            // === 水平位置计算 ===
            // 左侧：方向键组
            s32 dpadLeftX = x + 22;      // 方向键左（25 × 0.9 ≈ 22）
            s32 dpadCenterX = x + 54;    // 方向键上/下（60 × 0.9 = 54）
            s32 dpadRightX = x + 86;     // 方向键右（95 × 0.9 ≈ 86）
            
            // L/ZL列：在方向键右的右边，保持间距
            s32 lColumnX = dpadRightX + 32;  // L和ZL垂直对齐（35 × 0.9 ≈ 32）
            
            // 右侧：ABXY按键组
            s32 aButtonX = x + w - 40;    // A按键（45 × 0.9 ≈ 40）
            s32 rightColumnX = x + w - 72;  // X和B按键（80 × 0.9 = 72）
            s32 yButtonX = x + w - 104;   // Y按键（115 × 0.9 ≈ 104）
            
            // R/ZR列：在Y左边，保持间距
            s32 rColumnX = yButtonX - 32;  // R和ZR垂直对齐（35 × 0.9 ≈ 32）
            
            // === 垂直位置计算 ===
            const s32 layoutTotalHeight = 7 * buttonSize + 6 * rowSpacing;
            
            // 垂直居中
            s32 baseY = y + (h - layoutTotalHeight) / 2 + buttonSize - 10;
            
            // === 定义 drawButton 函数（根据映射显示按键）===
            auto drawButton = [&](const char* sourceName, s32 posX, s32 posY) {
                // 查找映射
                const char* targetIcon = nullptr;
                const char* targetName = nullptr;
                bool isMapped = false;
                
                for (int i = 0; i < MappingDef::BUTTON_COUNT; i++) {
                    if (strcmp(s_buttonMappings[i].source, sourceName) == 0) {
                        targetIcon = HidHelper::getButtonIcon(s_buttonMappings[i].target);
                        targetName = s_buttonMappings[i].target;
                        isMapped = (strcmp(s_buttonMappings[i].source, s_buttonMappings[i].target) != 0);
                        break;
                    }
                }
                
                if (!targetIcon) return;
                
                // 根据是否映射选择颜色
                tsl::Color color = isMapped ? blueColor : whiteColor;
                
                // 绘制图标
                renderer->drawString(targetIcon, false, posX, posY, buttonSize, 
                                    renderer->a(color));
                
                // 检查目标按键是否有连发，绘制黄色小点
                u64 flag = HidHelper::getButtonFlag(targetName);
                if (m_textAreaInfo.buttons & flag) {
                    s32 dotX = posX + buttonSize - 3;  // 往右移动 2（-5 → -3）
                    s32 dotY = posY - buttonSize + 3;  // 往上移动 2（+5 → +3）
                    renderer->drawCircle(dotX, dotY, 3, true, renderer->a(yellowColor));
                }
                
                // 检查目标按键是否有宏，绘制红色小点
                if (m_macroHotKey & flag) {
                    s32 macroX = posX + buttonSize - 3 + 8;      
                    s32 macroY = posY - buttonSize + 3; 
                    renderer->drawCircle(macroX, macroY, 3, true, renderer->a(redColor));
                }
            };
            
            // === 绘制按键布局 ===
            // 行1: ZL, ZR
            s32 row1Y = baseY;
            drawButton("ZL", lColumnX, row1Y);
            drawButton("ZR", rColumnX, row1Y);
            
            // 行2: L, R
            s32 row2Y = baseY + buttonSize + rowSpacing;
            drawButton("L", lColumnX, row2Y + 9);
            drawButton("R", rColumnX, row2Y + 9);
            
            // Select(-) 和 Start(+)：Y坐标在 ZR 和 R 之间
            s32 selectStartY = (row1Y + (row2Y + 9)) / 2;
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

        } else {
            // 不在游戏中，绘制相关信息（使用红色，水平和垂直居中）
            const char* noGameText = SysModuleManager::isRunning() ? "未检测到游戏，功能不可用" : "未启动系统模块，功能不可用";
            const s32 fontSize = 26;
            auto textDim = renderer->getTextDimensions(noGameText, false, fontSize);
            s32 centeredX = x + (w - textDim.first) / 2;
            s32 centeredY = y + (h + textDim.second) / 2;
            renderer->drawString(noGameText, false, centeredX, centeredY, fontSize, tsl::Color(0xF, 0x5, 0x5, 0xF));
        }
        
    });
    // 使用动态计算的文本区域高度
    mainList->addItem(textArea, TextAreaHeight);

    // ============= 下半部分：列表区域 =============
    // 创建开启连发列表项
    m_AutoFireEnableItem = new tsl::elm::ListItem("按键连发", m_textAreaInfo.isAutoFireEnabled ? "已开启" : "已关闭");
    m_AutoFireEnableItem->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            AutoKeyToggle();
            return true;
        }
        return false;
    });
    mainList->addItem(m_AutoFireEnableItem);

    m_AutoRemapEnableItem = new tsl::elm::ListItem("按键映射", m_textAreaInfo.isAutoRemapEnabled ? "已开启" : "已关闭");
    m_AutoRemapEnableItem->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            AutoRemapToggle();
            return true;
        }
        return false;
    });
    mainList->addItem(m_AutoRemapEnableItem);

    // 创建设置列表项
    m_AutoMacroEnableItem = new tsl::elm::ListItem("按键脚本", m_textAreaInfo.isAutoMacroEnabled ? "已开启" : "已关闭");
    m_AutoMacroEnableItem->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            AutoMacroToggle();
            return true;
        }
        return false;
    });
    mainList->addItem(m_AutoMacroEnableItem);

    // 创建切换配置列表项
    auto ConfigSwitchItem = new tsl::elm::ListItem("切换配置",">");
    ConfigSwitchItem->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            ConfigToggle();
            return true;
        }
        return false;
    });
    mainList->addItem(ConfigSwitchItem);
    
    // 将主列表设置为框架的内容
    frame->setContent(mainList);

    // 返回创建的界面元素
    return frame;
}

void MainMenu::update() {
    // 刷新机制
    if (Refresh::RefrConsume(Refresh::MainMenu)) RefreshData();
}

// 处理输入事件
bool MainMenu::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_Right) {
        tsl::changeTo<SettingMenu>();
        return true;
    }
    else if (keysDown & HidNpadButton_Left) {
        tsl::changeTo<AboutPlugin>();
        return true;
    }
    return false;
}