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

// 静态成员变量定义
TextAreaInfo MainMenu::s_TextAreaInfo;

// 全局指针：用于在 UpdateMainMenu 中更新
static tsl::elm::ListItem* g_EnableItem = nullptr;          // 开启连发列表项

// 静态方法：更新一次关键信息
void MainMenu::UpdateMainMenu() {
    
    // 获取当前运行程序的Title ID
    u64 currentTitleId = GameMonitor::getCurrentTitleId();
    
    // 检查是否在游戏中
    s_TextAreaInfo.isInGame = (currentTitleId != 0);
    
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
    if (s_TextAreaInfo.isInGame) {
        std::string GameConfigPath = "/config/AutoKeyLoop/GameConfig/" + std::string(s_TextAreaInfo.gameId) + ".ini";
        s_TextAreaInfo.GameConfigPath = GameConfigPath;
        if(ult::isFile(GameConfigPath)) {
            s_TextAreaInfo.isGlobalConfig = IniHelper::getBool("AUTOFIRE", "globconfig", true, GameConfigPath);
            s_TextAreaInfo.isAutoEnabled = IniHelper::getBool("AUTOFIRE", "autoenable", false, GameConfigPath);
        } else s_TextAreaInfo.isAutoEnabled = IniHelper::getBool("AUTOFIRE", "autoenable", false, CONFIG_PATH);
    }

    // 更新开启连发状态
    if (g_EnableItem != nullptr) g_EnableItem->setValue(s_TextAreaInfo.isAutoEnabled ? "已开启" : "已关闭");

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

    // 创建覆盖层框架，设置标题和版本（使用宏定义的版本号）
    auto frame = new tsl::elm::OverlayFrame("按键连发", APP_VERSION_STRING);

    // 创建主列表容器
    auto mainList = new tsl::elm::List();

    // ============= 上半部分：纯文本显示区域 =============
    // 创建自定义绘制器来显示纯文本内容（使用结构体成员数量）
    auto textArea = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        // 动态计算
        int TextLineCount = s_TextAreaInfo.isInGame ? 3 : 1;
        s32 TextFontSize = 22;
        s32 totalTextHeight = TextLineCount * TextFontSize + (TextLineCount - 1) * SPACING;  // 总文本高度
        s32 textStartY = y + (h - totalTextHeight) / 2;  // 垂直居中计算
        
        // 格式化信息文本
        char tempText[128];
        
        // 如果在游戏中，绘制游戏名称、ID和配置类型
        if (s_TextAreaInfo.isInGame) {

            // 第1行：游戏名称
            snprintf(tempText, sizeof(tempText), "名称：%s", s_TextAreaInfo.gameName);
            renderer->drawStringWithColoredSections(std::string(tempText), {"名称："}, x + SPACING, textStartY, 
                                                    TextFontSize, tsl::style::color::ColorText, tsl::style::color::ColorHighlight);
            
            // 第2行：游戏ID（修改为"编号"）
            snprintf(tempText, sizeof(tempText), "编号：%s", s_TextAreaInfo.gameId);
            renderer->drawStringWithColoredSections(std::string(tempText), {"编号："}, x + SPACING, textStartY + (TextFontSize + SPACING), 
                                                    TextFontSize, tsl::style::color::ColorText, tsl::style::color::ColorHighlight);
            
            // 第3行：配置类型
            snprintf(tempText, sizeof(tempText), "配置：%s", s_TextAreaInfo.isGlobalConfig ? "全局配置" : "独立游戏配置");
            renderer->drawStringWithColoredSections(std::string(tempText), {"配置："}, x + SPACING, textStartY + 2 * (TextFontSize + SPACING), 
                                                    TextFontSize, tsl::style::color::ColorText, tsl::style::color::ColorHighlight);

        } else {
            // 不在游戏中，绘制相关信息（使用红色，水平和垂直居中）
            const char* noGameText = "未检测到游戏，功能不可用！";
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
            
            s32 centeredX = x + (w - textLength * fontSize) / 2;
            
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