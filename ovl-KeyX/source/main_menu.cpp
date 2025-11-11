#include "main_menu.hpp"
#include "game_monitor.hpp"
#include "ini_helper.hpp"
#include "ipc_manager.hpp"
#include "sysmodule_manager.hpp"
#include "setting/setting_menu.hpp"
#include <ultra.hpp>

// Tesla插件界面尺寸常量定义
#define TESLA_VIEW_HEIGHT 720      // Tesla插件总高度
#define TESLA_TITLE_HEIGHT 97      // 标题栏高度
#define TESLA_BOTTOM_HEIGHT 73     // 底部按钮栏高度
#define LIST_ITEM_HEIGHT 70        // 列表项高度
#define SPACING 20  // 间距常量

constexpr const char* CONFIG_PATH = "/config/KeyX/config.ini";

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
    
    // 根据按键名称获取按键标志（用于检查连发位掩码）
    u64 getButtonFlag(const char* buttonName) {
        if (strcmp(buttonName, "A") == 0) return HidNpadButton_A;
        if (strcmp(buttonName, "B") == 0) return HidNpadButton_B;
        if (strcmp(buttonName, "X") == 0) return HidNpadButton_X;
        if (strcmp(buttonName, "Y") == 0) return HidNpadButton_Y;
        if (strcmp(buttonName, "Up") == 0) return HidNpadButton_Up;
        if (strcmp(buttonName, "Down") == 0) return HidNpadButton_Down;
        if (strcmp(buttonName, "Left") == 0) return HidNpadButton_Left;
        if (strcmp(buttonName, "Right") == 0) return HidNpadButton_Right;
        if (strcmp(buttonName, "L") == 0) return HidNpadButton_L;
        if (strcmp(buttonName, "R") == 0) return HidNpadButton_R;
        if (strcmp(buttonName, "ZL") == 0) return HidNpadButton_ZL;
        if (strcmp(buttonName, "ZR") == 0) return HidNpadButton_ZR;
        if (strcmp(buttonName, "StickL") == 0) return HidNpadButton_StickL;
        if (strcmp(buttonName, "StickR") == 0) return HidNpadButton_StickR;
        if (strcmp(buttonName, "Start") == 0) return HidNpadButton_Plus;
        if (strcmp(buttonName, "Select") == 0) return HidNpadButton_Minus;
        return 0;
    }
}

// 静态成员变量定义
TextAreaInfo MainMenu::s_TextAreaInfo;
static tsl::elm::ListItem* s_AutoFireEnableItem = nullptr;          // 开启连发列表项
static tsl::elm::ListItem* s_AutoRemapEnableItem = nullptr;          // 开启映射列表项


// 静态方法：更新一次关键信息
void MainMenu::UpdateMainMenu() {
    
    // 开局检查游戏TID
    // 目标是根据游戏TID，确认当前是否在游戏中
    // 如果不在游戏中，那配置文件使用全局配置
    // 如果在游戏中，那配置文件根据游戏配置文件中的设置来决定

    u64 currentTitleId = GameMonitor::getCurrentTitleId();
    s_TextAreaInfo.isInGame = (currentTitleId != 0) && SysModuleManager::isRunning();
    snprintf(s_TextAreaInfo.gameId, sizeof(s_TextAreaInfo.gameId), "%016lX", currentTitleId);
    s_TextAreaInfo.GameConfigPath = "/config/KeyX/GameConfig/" + std::string(s_TextAreaInfo.gameId) + ".ini";
    s_TextAreaInfo.isGlobalConfig = IniHelper::getBool("AUTOFIRE", "globconfig", true, s_TextAreaInfo.GameConfigPath);
    std::string SwitchConfigPath = (s_TextAreaInfo.isInGame && ult::isFile(s_TextAreaInfo.GameConfigPath)) ? s_TextAreaInfo.GameConfigPath : CONFIG_PATH;
    s_TextAreaInfo.isAutoFireEnabled = IniHelper::getBool("AUTOFIRE", "autoenable", false, SwitchConfigPath);
    s_TextAreaInfo.isAutoRemapEnabled = IniHelper::getBool("MAPPING", "autoenable", false, SwitchConfigPath);
    if (s_AutoFireEnableItem != nullptr) s_AutoFireEnableItem->setValue(s_TextAreaInfo.isAutoFireEnabled ? "已开启" : "已关闭");
    if (s_AutoRemapEnableItem != nullptr) s_AutoRemapEnableItem->setValue(s_TextAreaInfo.isAutoRemapEnabled ? "已开启" : "已关闭");

    // 读取按钮掩码值用于绘制按钮图标
    RefreshButtonsConfig();
}

// 刷新按钮配置，主要用于UI绘制
void MainMenu::RefreshButtonsConfig() {
    std::string ConfigPath = s_TextAreaInfo.isGlobalConfig ? CONFIG_PATH : s_TextAreaInfo.GameConfigPath;
    std::string buttonsStr = IniHelper::getString("AUTOFIRE", "buttons", "0", ConfigPath);
    s_TextAreaInfo.buttons = std::stoull(buttonsStr);
    // 读取映射配置
    for (int i = 0; i < BUTTON_COUNT; i++) {
        std::string temp = IniHelper::getString("MAPPING", s_ButtonMappings[i].source,
                                                 s_ButtonMappings[i].source, ConfigPath);
        strncpy(s_ButtonMappings[i].target, temp.c_str(), 7);
        s_ButtonMappings[i].target[7] = '\0';
    }
}

// 连发功能开关
void MainMenu::AutoKeyToggle() {
    if (!s_TextAreaInfo.isInGame) return;

    // 根据状态发送命令
    Result rc = s_TextAreaInfo.isAutoFireEnabled 
        ? g_ipcManager.sendDisableAutoFireCommand()   // 关闭
        : g_ipcManager.sendEnableAutoFireCommand();   // 开启

    if (R_FAILED(rc)) {
        s_AutoFireEnableItem->setValue("IPC通信失败");
        return;
    }

    // 切换状态，开变关-关变开
    s_TextAreaInfo.isAutoFireEnabled = !s_TextAreaInfo.isAutoFireEnabled;
    s_AutoFireEnableItem->setValue(s_TextAreaInfo.isAutoFireEnabled ? "已开启" : "已关闭");
    IniHelper::setBool("AUTOFIRE", "globconfig", s_TextAreaInfo.isGlobalConfig, s_TextAreaInfo.GameConfigPath);
    IniHelper::setBool("AUTOFIRE", "autoenable", s_TextAreaInfo.isAutoFireEnabled, s_TextAreaInfo.GameConfigPath);
}

// 映射功能开关
void MainMenu::AutoRemapToggle() {
    if (!s_TextAreaInfo.isInGame) return;

    // 根据状态发送命令
    Result rc = s_TextAreaInfo.isAutoRemapEnabled 
        ? g_ipcManager.sendDisableMappingCommand()   // 关闭
        : g_ipcManager.sendEnableMappingCommand();   // 开启
        
    if (R_FAILED(rc)) {
        s_AutoRemapEnableItem->setValue("IPC通信失败");
        return;
    }

    // 切换状态，开变关-关变开
    s_TextAreaInfo.isAutoRemapEnabled = !s_TextAreaInfo.isAutoRemapEnabled;
    s_AutoRemapEnableItem->setValue(s_TextAreaInfo.isAutoRemapEnabled ? "已开启" : "已关闭");
    IniHelper::setBool("AUTOFIRE", "globconfig", s_TextAreaInfo.isGlobalConfig, s_TextAreaInfo.GameConfigPath);
    IniHelper::setBool("MAPPING", "autoenable", s_TextAreaInfo.isAutoRemapEnabled, s_TextAreaInfo.GameConfigPath);
}

// 配置切换（全局/独立）
void MainMenu::ConfigToggle() {
    if (!s_TextAreaInfo.isInGame) return;
    s_TextAreaInfo.isGlobalConfig = !s_TextAreaInfo.isGlobalConfig;
    IniHelper::setBool("AUTOFIRE", "globconfig", s_TextAreaInfo.isGlobalConfig, s_TextAreaInfo.GameConfigPath);
    IniHelper::setBool("AUTOFIRE", "autoenable", s_TextAreaInfo.isAutoFireEnabled, s_TextAreaInfo.GameConfigPath);
    IniHelper::setBool("MAPPING", "autoenable", s_TextAreaInfo.isAutoRemapEnabled, s_TextAreaInfo.GameConfigPath);
    
    // 刷新界面显示
    UpdateMainMenu();

    // 发送重载配置命令
    Result rc = g_ipcManager.sendReloadBasicCommand();
    if (R_FAILED(rc)) {
        s_AutoFireEnableItem->setValue("IPC通信失败");
        s_AutoRemapEnableItem->setValue("IPC通信失败");
        return;
    }
}

// 主菜单构造函数
MainMenu::MainMenu()
{
    // 初始化时更新文本区域信息
    UpdateMainMenu();
}

// 创建用户界面
tsl::elm::Element* MainMenu::createUI()
{  
    // 动态计算文本区域高度
    s32 TextAreaHeight = (TESLA_VIEW_HEIGHT - TESLA_TITLE_HEIGHT - TESLA_BOTTOM_HEIGHT) - (4 * LIST_ITEM_HEIGHT) - SPACING * 2;

    // 创建覆盖层框架，使用自定义头部（指定高度97避免滚动条）
    auto frame = new tsl::elm::HeaderOverlayFrame(TESLA_TITLE_HEIGHT);
    
    // 自定义头部绘制器
    frame->setHeader(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        // 左侧：标题和版本
        renderer->drawString("按键助手", false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString(APP_VERSION_STRING, false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
        
        // 右侧：两行信息（往左移，避免截断，屏幕可见宽度约615px）
        // 垂直居中：高度97，行间距25，第一行Y=36，第二行Y=61
        renderer->drawString("TID :", false, 235, 36, 15, renderer->a(tsl::style::color::ColorText));
        renderer->drawString(s_TextAreaInfo.gameId, false, 275, 36, 15, renderer->a(tsl::style::color::ColorHighlight));
        
        renderer->drawString("配置:", false, 235, 61, 15, renderer->a(tsl::style::color::ColorText));
        const char* config = s_TextAreaInfo.isGlobalConfig ? "全局配置" : "独立配置";
        renderer->drawString(config, false, 275, 61, 15, renderer->a(tsl::style::color::ColorHighlight));

        // 底部按钮（使用绝对坐标）
        renderer->drawString("\uE0EE  设置", false, 280, 693, 23, renderer->a(tsl::style::color::ColorText));

    }));

    // 创建主列表容器
    auto mainList = new tsl::elm::List();

    // ============= 上半部分：纯文本显示区域 =============
    // 创建自定义绘制器来显示纯文本内容（使用结构体成员数量）
    auto textArea = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        
        
        // 如果在游戏中，绘制按键图标
        if (s_TextAreaInfo.isInGame) {
            tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};          // 白色：未映射
            tsl::Color blueColor = {0x00, 0xDD, 0xFF, 0xFF};           // 亮天蓝色：已映射
            tsl::Color yellowColor = {0xFF, 0xFF, 0x00, 0xFF};         // 黄色：连发小点
            
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
                
                for (int i = 0; i < BUTTON_COUNT; i++) {
                    if (strcmp(s_ButtonMappings[i].source, sourceName) == 0) {
                        targetIcon = getButtonIcon(s_ButtonMappings[i].target);
                        targetName = s_ButtonMappings[i].target;
                        isMapped = (strcmp(s_ButtonMappings[i].source, s_ButtonMappings[i].target) != 0);
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
                u64 flag = getButtonFlag(targetName);
                if (s_TextAreaInfo.buttons & flag) {
                    s32 dotX = posX + buttonSize - 3;  // 往右移动 2（-5 → -3）
                    s32 dotY = posY - buttonSize + 3;  // 往上移动 2（+5 → +3）
                    renderer->drawCircle(dotX, dotY, 3, true, renderer->a(yellowColor));
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
    s_AutoFireEnableItem = new tsl::elm::ListItem("按键连发", s_TextAreaInfo.isAutoFireEnabled ? "已开启" : "已关闭");
    s_AutoFireEnableItem->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            MainMenu::AutoKeyToggle();
            return true;
        }
        return false;
    });
    mainList->addItem(s_AutoFireEnableItem);

    s_AutoRemapEnableItem = new tsl::elm::ListItem("按键映射", s_TextAreaInfo.isAutoRemapEnabled ? "已开启" : "已关闭");
    s_AutoRemapEnableItem->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            MainMenu::AutoRemapToggle();
            return true;
        }
        return false;
    });
    mainList->addItem(s_AutoRemapEnableItem);

    // 创建设置列表项
    auto listItemMacro = new tsl::elm::ListItem("按键脚本","已关闭");
    listItemMacro->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            return true;
        }
        return false;
    });
    mainList->addItem(listItemMacro);

    // 创建切换配置列表项
    auto ConfigSwitchItem = new tsl::elm::ListItem("切换配置",">");
    ConfigSwitchItem->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            MainMenu::ConfigToggle();
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