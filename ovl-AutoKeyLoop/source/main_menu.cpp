#include "main_menu.hpp"
#include "game_monitor.hpp"
#include "setting_manager.hpp"

// Tesla插件界面尺寸常量定义
#define TESLA_VIEW_HEIGHT 720      // Tesla插件总高度
#define TESLA_TITLE_HEIGHT 97      // 标题栏高度
#define TESLA_BOTTOM_HEIGHT 73     // 底部按钮栏高度
#define LIST_ITEM_HEIGHT 70        // 列表项高度
#define SPACING 20  // 间距常量


// 静态成员变量定义
TextAreaInfo MainMenu::s_TextAreaInfo;

// 全局指针：用于在 UpdateTextAreaInfo 中更新
static tsl::elm::ListItem* g_EnableItem = nullptr;          // 开启连发列表项
static tsl::elm::ListItemV2* g_ConfigSwitchItem = nullptr;  // 切换配置列表项

// 静态方法：更新文本区域信息
void MainMenu::UpdateTextAreaInfo() {
    
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
    
    // 如果 ListItem 已创建，则同步更新 UI
    
    // 更新开启连发状态
    if (g_EnableItem != nullptr) g_EnableItem->setValue(s_TextAreaInfo.isInGame ? "已关闭" : "不可用");
    
    // 更新切换配置状态
    if (g_ConfigSwitchItem != nullptr) {
        g_ConfigSwitchItem->setValue(s_TextAreaInfo.isInGame ? ">" : "不可用");
        if (!s_TextAreaInfo.isInGame) g_ConfigSwitchItem->setValueColor({0xF, 0x5, 0x5, 0xF}); // 红色
        else g_ConfigSwitchItem->setValueColor({0x8, 0xC, 0xF, 0xF}); // 标准蓝色
    }
    
}

// 主菜单构造函数
MainMenu::MainMenu()
{
    // 初始化时更新文本区域信息
    UpdateTextAreaInfo();
}

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
            snprintf(tempText, sizeof(tempText), "配置：%s", s_TextAreaInfo.isGlobalConfig ? "全局" : "独立");
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
    g_EnableItem = new tsl::elm::ListItem("开启连发", s_TextAreaInfo.isInGame ? "已关闭" : "已关闭");
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
    g_ConfigSwitchItem = new tsl::elm::ListItemV2("切换配置", s_TextAreaInfo.isInGame ? ">" : "不可用");
    
    // 如果不在游戏中，设置值为红色；否则使用标准蓝色
    if (!s_TextAreaInfo.isInGame) g_ConfigSwitchItem->setValueColor({0xF, 0x5, 0x5, 0xF}); // 红色
    else g_ConfigSwitchItem->setValueColor({0x8, 0xC, 0xF, 0xF}); // 标准蓝色
    
    mainList->addItem(g_ConfigSwitchItem);
 
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