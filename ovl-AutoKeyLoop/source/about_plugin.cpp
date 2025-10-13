#include "about_plugin.hpp"

// 关于插件界面构造函数
AboutPlugin::AboutPlugin()
{
}

// 创建关于插件界面UI
tsl::elm::Element* AboutPlugin::createUI()
{
    // 创建覆盖层框架，设置标题和副标题
    auto frame = new tsl::elm::OverlayFrame("关于插件", APP_VERSION_STRING);

    // 创建主列表容器
    auto mainList = new tsl::elm::List();

    // 创建带有视觉效果的文本显示区域
    auto aboutText = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        // 定义关于插件的文本内容
        const char* aboutInfo[] = {
            "AutoKeyLoop",
            " • 按键连发",
            "",
            "作者：",
            " • TOM",
            "",
            "鹅群：",
            " • 1051287661",
            "",
            "功能介绍：",
            " • 支持按键自动连发功能",
            " • 可自定义连发速度和间隔",
            " • 提供全局和游戏专用配置",
            "",
            "使用说明：",
            " • 进入游戏后打开本插件",
            " • 游戏设置调整连发参数",
            " • 点击保存设置",
            " • 点击开启连发",
            "",
            "感谢使用本插件！"
        };
        
        const size_t lineCount = sizeof(aboutInfo) / sizeof(aboutInfo[0]);
        const s32 lineHeight = 22;  // 增大行高
        const s32 fontSize = 20;    // 整体字号加大
        const s32 titleFontSize = 24;  // 0、1行字体更大
        const s32 padding = 20;
        
        // 计算文本总高度
        s32 totalTextHeight = lineCount * lineHeight;
        
        // 计算垂直居中的起始位置
        s32 startY = y + (h - totalTextHeight) / 2;
        
        // 绘制文本内容
        for (size_t i = 0; i < lineCount; i++) {
            s32 textY = startY + i * lineHeight;
            if (textY + lineHeight > y + h - 10) break; // 防止超出显示区域
            
            // 根据行号和内容类型选择颜色和字体大小
            tsl::Color textColor = {0xFF, 0xFF, 0xFF, 0xFF}; // 默认白色
            s32 currentFontSize = fontSize;
            s32 textX = x + padding;
            
            if (i == 0) { // 第0行：亮蓝色，大字体
                textColor = {0x66, 0xCC, 0xFF, 0xFF}; // 亮蓝色
                currentFontSize = titleFontSize;
            } else if (i == 1) { // 第1行：白色，大字体
                textColor = {0xFF, 0xFF, 0xFF, 0xFF}; // 白色
                currentFontSize = fontSize;
            } else if (strstr(aboutInfo[i], "：") != nullptr || strstr(aboutInfo[i], "！") != nullptr ) { // 带冒号的行：亮蓝色
                textColor = {0x66, 0xCC, 0xFF, 0xFF}; // 亮蓝色
            } else { // 不带冒号的行：白色
                textColor = {0xFF, 0xFF, 0xFF, 0xFF}; // 白色
            }
            
            renderer->drawString(aboutInfo[i], false, textX, textY, currentFontSize, textColor);
        }
    });
    
    // 设置文本区域高度（占满整个内容区域）
    mainList->addItem(aboutText, 550);

    // 将主列表设置为框架的内容
    frame->setContent(mainList);

    // 返回创建的界面元素
    return frame;
}