#include "macro_view.hpp"
#include "macro_rename.hpp"
#include "macro_list.hpp"
#include "macro_record.hpp" 
#include <ultra.hpp>

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

// 脚本查看类
MacroViewGui::MacroViewGui(const char* macroFilePath, const char* gameName) 
 : m_info()
{
    
    strcpy(m_macroFilePath, macroFilePath);
    strcpy(m_info.gameName, gameName);
    std::string fileName = ult::getFileName(macroFilePath);
    auto dot = fileName.rfind('.');
    if (dot != std::string::npos) fileName = fileName.substr(0, dot); 
    strcpy(m_info.macroName, fileName.c_str());
    struct stat st{};
    if (stat(macroFilePath, &st) == 0) {
        u32 kb = static_cast<u32>((st.st_size + 1023) / 1024);
        snprintf(m_info.fileSizeKb, sizeof(m_info.fileSizeKb), "%u KB", static_cast<u16>(kb));
    }
    ParsingMacros();
    
}

// 解析宏文件，获取文件头数据
void MacroViewGui::ParsingMacros(){
    FILE* fp = fopen(m_macroFilePath, "rb");
    if (!fp) return;

    MacroHeader header{};
    if (fread(&header, sizeof(header), 1, fp) == 1) {
        snprintf(m_info.titleId, sizeof(m_info.titleId), "%016lX", header.titleId);
        snprintf(m_info.totalFrames, sizeof(m_info.totalFrames), "%u", header.frameCount);
        snprintf(m_info.fps, sizeof(m_info.fps), "%u FPS", header.frameRate);
        snprintf(m_info.durationSec, sizeof(m_info.durationSec), "%u s", header.frameRate ? static_cast<u8>(header.frameCount / header.frameRate) : 0);
    }
    fclose(fp);
}


tsl::elm::Element* MacroViewGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame(m_info.gameName, "管理录制的脚本");
    auto list = new tsl::elm::List();

    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        constexpr const char* kLabels[] = {
            "游戏名称：", "游戏编号：", "脚本名称：",
            "脚本大小：", "录制时间：", "录制帧率：", "录制帧数："
        };

        const char* values[] = {
            m_info.gameName,
            m_info.titleId,
            m_info.macroName,
            m_info.fileSizeKb,
            m_info.durationSec,
            m_info.fps,
            m_info.totalFrames
        };
        
        char line[64];
        constexpr const u32 fontSize = 20;
        const s32 startX = x + 26;
        s32 lineHeight = r->getTextDimensions("你", false, fontSize).second + 15;
        s32 totalHeight = lineHeight * static_cast<s32>(std::size(kLabels));
        s32 startY = y + (h - totalHeight) / 2 - 20;
        for (size_t i = 0; i < std::size(kLabels); ++i) {
            snprintf(line, sizeof(line), "%s%s", kLabels[i], values[i]);
            r->drawStringWithColoredSections(
                line,
                false,
                {kLabels[i]},
                startX,
                startY + lineHeight,
                fontSize,
                tsl::Color(0xF, 0xF, 0xF, 0xF),  
                tsl::highlightColor2               
            );
            startY += lineHeight;
        }
    });
    list->addItem(textArea, 310);

    auto listButton = new tsl::elm::ListItem("分配按键",">");
    listButton->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            return true;
        }
        return false;
    });
    list->addItem(listButton);

    auto listChangeName = new tsl::elm::ListItem("修改名称",">");
    listChangeName->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<MacroRenameGui>(m_macroFilePath, m_info.gameName);
            return true;
        }   
        return false;
    });
    list->addItem(listChangeName);

    m_deleteItem = new tsl::elm::ListItem("删除脚本","按  删除");
    list->addItem(m_deleteItem);

    frame->setContent(list);
    return frame;
}

bool MacroViewGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if ((keysDown & HidNpadButton_Minus) && m_deleteItem) {
        if (getFocusedElement() == m_deleteItem) {
            ult::deleteFileOrDirectory(m_macroFilePath);
            tsl::goBack();
            tsl::goBack();
            u64 tid = strtoull(m_info.titleId, nullptr, 16);
            tsl::changeTo<MacroListGuiGame>(tid);
            return true;
        }
    }

    return false;
}

