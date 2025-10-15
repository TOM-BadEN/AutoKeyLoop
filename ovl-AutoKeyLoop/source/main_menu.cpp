#include "main_menu.hpp"
#include "game_monitor.hpp"
#include "setting_manager.hpp"
#include "ini_helper.hpp"
#include "ipc_manager.hpp"
#include "sysmodule_manager.hpp"
#include <ultra.hpp>

// Tesla插件界面尺寸常量定义
#define TESLA_VIEW_HEIGHT 720      // Tesla插件总高度
#define TESLA_TITLE_HEIGHT 97      // 标题栏高度
#define TESLA_BOTTOM_HEIGHT 73     // 底部按钮栏高度
#define LIST_ITEM_HEIGHT 70        // 列表项高度
#define SPACING 20  // 间距常量

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

// 静态成员变量定义
TextAreaInfo MainMenu::s_TextAreaInfo;

// 全局指针：用于在 UpdateMainMenu 中更新
static tsl::elm::ListItem* g_EnableItem = nullptr;          // 开启连发列表项


// 静态方法：更新一次关键信息
void MainMenu::UpdateMainMenu() {
    
    // 获取当前运行程序的Title ID
    u64 currentTitleId = GameMonitor::getCurrentTitleId();
    
    // 检查是否满足运行条件
    s_TextAreaInfo.isInGame = (currentTitleId != 0) && SysModuleManager::isRunning();
    
    // 将Title ID转换为16位十六进制字符串
    snprintf(s_TextAreaInfo.gameId, sizeof(s_TextAreaInfo.gameId), "%016lX", currentTitleId);
    
    // 获取游戏名称，写入到结构体中
    GameMonitor::getTitleIdGameName(currentTitleId, s_TextAreaInfo.gameName);

    // 默认使用全局配置
    s_TextAreaInfo.isGlobalConfig = true;
    // 默认是显示连发不开启
    s_TextAreaInfo.isAutoEnabled = false;

    /**
        逻辑如下：
        1. 这里读取信息只为了更新ovl插件的UI，实际开启由系统模块自己控制
        2. 先看有没有独立配置文件，没有的话，就使用全局配置
        3. 如果有独立配置文件，则读取独立配置文件的内容
        4. 更新开启连发列表项的UI显示
    */
    std::string GameConfigPath = "/config/AutoKeyLoop/GameConfig/" + std::string(s_TextAreaInfo.gameId) + ".ini";
    if (s_TextAreaInfo.isInGame) {
        s_TextAreaInfo.GameConfigPath = GameConfigPath;
        if(ult::isFile(GameConfigPath)) {
            s_TextAreaInfo.isGlobalConfig = IniHelper::getBool("AUTOFIRE", "globconfig", true, GameConfigPath);
            s_TextAreaInfo.isAutoEnabled = IniHelper::getBool("AUTOFIRE", "autoenable", false, GameConfigPath);
        } else s_TextAreaInfo.isAutoEnabled = IniHelper::getBool("AUTOFIRE", "autoenable", false, CONFIG_PATH);
    }

    // 读取按钮掩码值用于绘制按钮图标
    RefreshButtonsConfig();
    
    // 更新开启连发状态
    if (g_EnableItem != nullptr) g_EnableItem->setValue(s_TextAreaInfo.isAutoEnabled ? "已开启" : "已关闭");

}

// 静态方法：刷新按钮配置（从配置文件中读取）
void MainMenu::RefreshButtonsConfig() {
    std::string GameConfigPath = "/config/AutoKeyLoop/GameConfig/" + std::string(s_TextAreaInfo.gameId) + ".ini";
    std::string buttonsStr;
    
    if (s_TextAreaInfo.isGlobalConfig) {
        buttonsStr = IniHelper::getString("AUTOFIRE", "buttons", "0", CONFIG_PATH);
    } else {
        buttonsStr = IniHelper::getString("AUTOFIRE", "buttons", "0", GameConfigPath);
    }
    
    s_TextAreaInfo.buttons = std::stoull(buttonsStr);
}

// 静态方法：连发功能开关
void MainMenu::AutoKeyToggle() {
    // 不在游戏中，不响应
    if (!s_TextAreaInfo.isInGame) return;

    // 检查系统模块是否在运行
    bool isRunning = SysModuleManager::isRunning();

    // 如果当前是关闭状态，并且系统模块未运行，则启动它
    if (!s_TextAreaInfo.isAutoEnabled && !isRunning) {
        Result rc = SysModuleManager::startModule();
        if (R_FAILED(rc)) {
            g_EnableItem->setValue("启动失败");
            return;
        }
        svcSleepThread(20000000ULL);  // 等待20ms初始化
        isRunning = true;  // 更新运行状态
    }
    
    // 如果当前是开启状态，并且系统模块未运行，则直接关闭
    if (s_TextAreaInfo.isAutoEnabled && !isRunning) {
        s_TextAreaInfo.isAutoEnabled = false;
        g_EnableItem->setValue("已关闭");
        return;
    }

    // 根据状态发送命令
    Result rc = s_TextAreaInfo.isAutoEnabled 
        ? g_ipcManager.sendDisableCommand()   // 关闭
        : g_ipcManager.sendEnableCommand();   // 开启

    if (R_FAILED(rc)) {
        g_EnableItem->setValue("通信失败");
        return;
    }

    // 更新状态
    s_TextAreaInfo.isAutoEnabled = !s_TextAreaInfo.isAutoEnabled;
    g_EnableItem->setValue(s_TextAreaInfo.isAutoEnabled ? "已开启" : "已关闭");
    
    // 保存配置到独立游戏配置文件
    IniHelper::setBool("AUTOFIRE", "globconfig", s_TextAreaInfo.isGlobalConfig, s_TextAreaInfo.GameConfigPath);
    IniHelper::setBool("AUTOFIRE", "autoenable", s_TextAreaInfo.isAutoEnabled, s_TextAreaInfo.GameConfigPath);
}

// 静态方法：配置切换（全局/独立）
void MainMenu::ConfigToggle() {
    // 不在游戏中，不响应
    if (!s_TextAreaInfo.isInGame) return;
    
    // 切换全局/独立配置
    s_TextAreaInfo.isGlobalConfig = !s_TextAreaInfo.isGlobalConfig;
    
    // 保存配置到独立游戏配置文件
    IniHelper::setBool("AUTOFIRE", "globconfig", s_TextAreaInfo.isGlobalConfig, s_TextAreaInfo.GameConfigPath);
    IniHelper::setBool("AUTOFIRE", "autoenable", s_TextAreaInfo.isAutoEnabled, s_TextAreaInfo.GameConfigPath);
    
    // 刷新按钮配置（切换配置后立即更新UI显示）
    RefreshButtonsConfig();
    
    // 如果连发功能未开启，则不重启
    if (!s_TextAreaInfo.isAutoEnabled) return;

    // 如果系统模块未运行，则启动它
    if (!SysModuleManager::isRunning()) {
        Result rc = SysModuleManager::startModule();
        if (R_FAILED(rc)) {
            g_EnableItem->setValue("启动失败");
            return;
        }
        svcSleepThread(20000000ULL);  // 等待20ms初始化
    }

    // 重启连发功能（让他重加载配置）
    Result rc = g_ipcManager.sendRestartCommand();
    if (R_FAILED(rc)) {
        g_EnableItem->setValue("通信失败");
        return;
    }
}

// 主菜单构造函数
MainMenu::MainMenu()
{
    // 初始化时更新文本区域信息
    UpdateMainMenu();
}

/*
    注意：ovl插件中关于连发开关已启动的功能，只是UI的更新，所以异常状态下，是有可能出现状态不同步的
    实际连发功能是否开启，还是需要看系统模块的真实状态，整个ovl中没有真正获取的方法，
    我不太想通过ipc去读取，感觉没必要。
*/

// 创建用户界面
tsl::elm::Element* MainMenu::createUI()
{  
    // 动态计算文本区域高度
    s32 TextAreaHeight = (TESLA_VIEW_HEIGHT - TESLA_TITLE_HEIGHT - TESLA_BOTTOM_HEIGHT) - (4 * LIST_ITEM_HEIGHT) - SPACING * 2;

    // 创建覆盖层框架，使用自定义头部（指定高度97避免滚动条）
    auto frame = new tsl::elm::HeaderOverlayFrame(TESLA_TITLE_HEIGHT);
    
    // 自定义头部绘制器
    frame->setHeader(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        // 左侧：标题和版本（参考 EdiZon 坐标）
        renderer->drawString("按键连发", false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString(APP_VERSION_STRING, false, 20, 50+23, 15, renderer->a(tsl::versionTextColor));
        
        // 右侧：两行信息（往左移，避免截断，屏幕可见宽度约615px）
        // 垂直居中：高度97，行间距25，第一行Y=36，第二行Y=61
        renderer->drawString("TID :", false, 235, 36, 15, renderer->a(tsl::style::color::ColorText));
        renderer->drawString(s_TextAreaInfo.gameId, false, 275, 36, 15, renderer->a(tsl::style::color::ColorHighlight));
        
        renderer->drawString("配置:", false, 235, 61, 15, renderer->a(tsl::style::color::ColorText));
        const char* config = s_TextAreaInfo.isGlobalConfig ? "全局配置" : "独立配置";
        renderer->drawString(config, false, 275, 61, 15, renderer->a(tsl::style::color::ColorHighlight));

    }));

    // 创建主列表容器
    auto mainList = new tsl::elm::List();

    // ============= 上半部分：纯文本显示区域 =============
    // 创建自定义绘制器来显示纯文本内容（使用结构体成员数量）
    auto textArea = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        
        
        // 如果在游戏中，绘制按键图标
        if (s_TextAreaInfo.isInGame) {
            tsl::Color whiteColor = {0xFF, 0xFF, 0xFF, 0xFF};
            tsl::Color lightBlueColor = {0x00, 0xDD, 0xFF, 0xFF};  // 亮天蓝色
            
            const s32 buttonSize = 25;   // 按钮大小（缩小）
            const s32 rowSpacing = 8;    // 行间距（缩小）
            
            // === 水平位置计算 ===
            // 左侧：方向键组
            s32 dpadLeftX = x + 35;      // 方向键左
            s32 dpadCenterX = x + 70;    // 方向键上/下
            s32 dpadRightX = x + 105;    // 方向键右
            
            // L/ZL列：在方向键右的右边，保持间距
            s32 lColumnX = dpadRightX + 35;  // L和ZL垂直对齐
            
            // 右侧：ABXY按键组（保持原位）
            s32 aButtonX = x + w - 35;    // A按键
            s32 rightColumnX = x + w - 70;  // X和B按键（垂直对齐）
            s32 yButtonX = x + w - 105;   // Y按键
            
            // R/ZR列：在Y左边，保持间距
            s32 rColumnX = yButtonX - 35;  // R和ZR垂直对齐
            
            // === 垂直位置计算 ===
            const s32 layoutTotalHeight = 5 * buttonSize + 4 * rowSpacing;
            
            // 垂直居中
            s32 baseY = y + (h - layoutTotalHeight) / 2 + buttonSize - 10;
            
            // === 根据 s_TextAreaInfo.buttons 显示按键颜色 ===
            // 行1: ZL, ZR
            s32 row1Y = baseY;
            renderer->drawString(ButtonIcon::ZL, false, lColumnX, row1Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_ZL) ? lightBlueColor : whiteColor));
            renderer->drawString(ButtonIcon::ZR, false, rColumnX, row1Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_ZR) ? lightBlueColor : whiteColor));
            
            // 行2: L, R
            s32 row2Y = baseY + buttonSize + rowSpacing;
            renderer->drawString(ButtonIcon::L, false, lColumnX, row2Y + 5, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_L) ? lightBlueColor : whiteColor));
            renderer->drawString(ButtonIcon::R, false, rColumnX, row2Y + 5, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_R) ? lightBlueColor : whiteColor));
            
            // 行3: 方向键上, X
            s32 row3Y = baseY + 2 * (buttonSize + rowSpacing);
            renderer->drawString(ButtonIcon::Up, false, dpadCenterX, row3Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_Up) ? lightBlueColor : whiteColor));
            renderer->drawString(ButtonIcon::X, false, rightColumnX, row3Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_X) ? lightBlueColor : whiteColor));
            
            // 行4: 方向键左右, Y/A
            s32 row4Y = baseY + 3 * (buttonSize + rowSpacing);
            renderer->drawString(ButtonIcon::Left, false, dpadLeftX, row4Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_Left) ? lightBlueColor : whiteColor));
            renderer->drawString(ButtonIcon::Right, false, dpadRightX, row4Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_Right) ? lightBlueColor : whiteColor));
            renderer->drawString(ButtonIcon::Y, false, yButtonX, row4Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_Y) ? lightBlueColor : whiteColor));
            renderer->drawString(ButtonIcon::A, false, aButtonX, row4Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_A) ? lightBlueColor : whiteColor));
            
            // 行5: 方向键下, B
            s32 row5Y = baseY + 4 * (buttonSize + rowSpacing);
            renderer->drawString(ButtonIcon::Down, false, dpadCenterX, row5Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_Down) ? lightBlueColor : whiteColor));
            renderer->drawString(ButtonIcon::B, false, rightColumnX, row5Y, buttonSize, 
                renderer->a((s_TextAreaInfo.buttons & HidNpadButton_B) ? lightBlueColor : whiteColor));

        } else {
            // 不在游戏中，绘制相关信息（使用红色，水平和垂直居中）
            const char* noGameText = SysModuleManager::isRunning() ? "未检测到游戏，功能不可用！" : "未启动系统模块，功能不可用！";
            const s32 fontSize = 26;
            
            // 动态计算垂直居中位置
            s32 centeredY = y + (h - fontSize) / 2;
            
            // 动态计算水平居中位置 - 计算实际字符数量
            s32 textLength = 0;
            for (const char* p = noGameText; *p; ) {
                if ((*p & 0x80) == 0) {
                    // ASCII字符
                    p++;
                    textLength++;
                } else if ((*p & 0xE0) == 0xC0) {
                    // UTF-8 2字节字符
                    p += 2;
                    textLength++;
                } else if ((*p & 0xF0) == 0xE0) {
                    // UTF-8 3字节字符（中文）
                    p += 3;
                    textLength++;
                } else if ((*p & 0xF8) == 0xF0) {
                    // UTF-8 4字节字符
                    p += 4;
                    textLength++;
                } else {
                    p++;
                }
            }
            
            s32 centeredX = x + (w - textLength * fontSize) / 2 + 5;
            
            renderer->drawString(noGameText, false, centeredX, centeredY, fontSize, tsl::Color(0xF, 0x5, 0x5, 0xF));
        }
        
    });
    // 使用动态计算的文本区域高度
    mainList->addItem(textArea, TextAreaHeight);

    // ============= 下半部分：列表区域 =============
    // 创建开启连发列表项
    g_EnableItem = new tsl::elm::ListItem("开启连发", s_TextAreaInfo.isAutoEnabled ? "已开启" : "已关闭");
    g_EnableItem->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            MainMenu::AutoKeyToggle();
            return true;
        }
        return false;
    });
    mainList->addItem(g_EnableItem);

    // 创建设置列表项
    auto listItemSetting = new tsl::elm::ListItem("连发设置",">");
    listItemSetting->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<AutoKeySetting>(s_TextAreaInfo.gameId, s_TextAreaInfo.gameName);
            return true;
        }
        return false;
    });
    mainList->addItem(listItemSetting);

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
 
    // 创建关于插件列表项
    auto listItemAbout = new tsl::elm::ListItem("关于插件",">");
    // 为关于插件列表项添加点击事件处理
    listItemAbout->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            // 切换到关于插件界面
            tsl::changeTo<AboutPlugin>();
            return true;
        }
        return false;
    });
    mainList->addItem(listItemAbout);
    
    // 将主列表设置为框架的内容
    frame->setContent(mainList);

    // 返回创建的界面元素
    return frame;
}