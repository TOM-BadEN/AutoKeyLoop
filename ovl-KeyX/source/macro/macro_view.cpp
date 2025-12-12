#include "macro_view.hpp"
#include "macro_rename.hpp"
#include "macro_list.hpp"
#include "macro_record.hpp" 
#include <ultra.hpp>
#include "ini_helper.hpp" 
#include "hiddata.hpp"
#include "refresh.hpp"
#include <algorithm>
#include "ipc.hpp"
#include "macro_edit.hpp"

namespace {

    // 删除指定脚本的快捷键配置
    bool removeHotkeyIfExists(const char* macroFilePath, const char* gameCfgPath) {
        // 获取配置文件中的宏数量
        int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, gameCfgPath);
        if (macroCount <= 0) return false;
        // 查找匹配项
        int foundIndex = -1;
        for (int i = 1; i <= macroCount; i++) {
            std::string key = "macro_path_" + std::to_string(i);
            std::string path = IniHelper::getString("MACRO", key, "", gameCfgPath);
            if (path == macroFilePath) {
                foundIndex = i;
                break;
            }
        }
        if (foundIndex == -1) return false;  // 未找到
        // 将后续项前移（压缩索引）
        for (int i = foundIndex; i < macroCount; i++) {
            // 当前项
            std::string pathKey = "macro_path_" + std::to_string(i);
            std::string comboKey = "macro_combo_" + std::to_string(i);
            // 下一项
            std::string nextPathKey = "macro_path_" + std::to_string(i + 1);
            std::string nextComboKey = "macro_combo_" + std::to_string(i + 1);
            // 读取下一项
            std::string nextPath = IniHelper::getString("MACRO", nextPathKey, "", gameCfgPath);
            int nextCombo = IniHelper::getInt("MACRO", nextComboKey, 0, gameCfgPath);
            // 覆盖当前项
            IniHelper::setString("MACRO", pathKey, nextPath, gameCfgPath);
            IniHelper::setInt("MACRO", comboKey, nextCombo, gameCfgPath);
        }
        // 删除最后一项的键
        std::string lastPathKey = "macro_path_" + std::to_string(macroCount);
        std::string lastComboKey = "macro_combo_" + std::to_string(macroCount);
        IniHelper::removeKey("MACRO", lastPathKey, gameCfgPath);
        IniHelper::removeKey("MACRO", lastComboKey, gameCfgPath);
        // 更新计数
        IniHelper::setInt("MACRO", "macroCount", macroCount - 1, gameCfgPath);
        return true;
    }


    constexpr const u64 buttons[] = {
        BTN_ZL,
        BTN_ZR,
        BTN_L,
        BTN_R,
        BTN_A,
        BTN_B,
        BTN_X,
        BTN_Y,
        BTN_UP,
        BTN_DOWN,
        BTN_LEFT,
        BTN_RIGHT,
        BTN_START,
        BTN_SELECT,
        BTN_STICKL,
        BTN_STICKR
    };
}

// 脚本查看类
MacroViewGui::MacroViewGui(const char* macroFilePath, const char* gameName, bool isRecord) 
 : m_isRecord(isRecord)
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
    getHotkey();

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

void MacroViewGui::getHotkey() {
    m_Hotkey = 0;
    sprintf(m_gameCfgPath, "sdmc:/config/KeyX/GameConfig/%s.ini", m_info.titleId);
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, m_gameCfgPath);
    for (int idx = 1; idx <= macroCount; ++idx) {
        std::string path  = "macro_path_"  + std::to_string(idx);
        std::string macroPath = IniHelper::getString("MACRO", path, "", m_gameCfgPath);
        if (macroPath == m_macroFilePath) {
            std::string combo = "macro_combo_" + std::to_string(idx);
            m_Hotkey = static_cast<u64>(IniHelper::getInt("MACRO", combo, 0, m_gameCfgPath));
        }
    }
}


tsl::elm::Element* MacroViewGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame(m_info.gameName, "管理录制的脚本");
    auto list = new tsl::elm::List();
    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        constexpr const char* kLabels[] = {
            "游戏编号：", "脚本名称：",
            "脚本大小：", "录制时间：", "录制帧率：", "录制帧数："
        };

        const char* values[] = {
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
        s32 lineHeight = r->getTextDimensions(ult::i18n("你"), false, fontSize).second + 15;
        s32 totalHeight = lineHeight * static_cast<s32>(std::size(kLabels));
        s32 startY = y + (h - totalHeight) / 2 - 20;
        for (size_t i = 0; i < std::size(kLabels); ++i) {
            snprintf(line, sizeof(line), "%s%s", ult::i18n(kLabels[i]).c_str(), values[i]);
            r->drawStringWithColoredSections(
                line,
                false,
                {ult::i18n(kLabels[i])},
                startX,
                startY + lineHeight,
                fontSize,
                tsl::Color(0xF, 0xF, 0xF, 0xF),  
                tsl::highlightColor2               
            );
            startY += lineHeight;
        }
    });
    list->addItem(textArea, 240);

    m_listButton = new tsl::elm::ListItem("分配按键", m_Hotkey ? HidHelper::getCombinedIcons(m_Hotkey) : ">");
    m_listButton->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<MacroHotKeySettingGui>(m_macroFilePath, m_info.gameName, m_gameCfgPath, m_Hotkey);
            return true;
        }
        return false;
    });
    list->addItem(m_listButton);

    auto listChangeName = new tsl::elm::ListItem("修改名称", ">");
    listChangeName->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<MacroRenameGui>(m_macroFilePath, m_info.gameName);
            return true;
        }   
        return false;
    });
    list->addItem(listChangeName);

    auto listEditMacro = new tsl::elm::ListItem("编辑脚本", ">");
    listEditMacro->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<MacroEditGui>(m_macroFilePath, m_info.gameName, m_isRecord);
            return true;
        }   
        return false;
    });  
    list->addItem(listEditMacro);


    m_deleteItem = new tsl::elm::ListItem("删除脚本","按  删除");
    list->addItem(m_deleteItem);

    frame->setContent(list);
    return frame;
}

void MacroViewGui::update() {
    if (Refresh::RefrConsume(Refresh::MacroView)) {
        getHotkey();
        m_listButton->setValue(m_Hotkey ? HidHelper::getCombinedIcons(m_Hotkey) : ">");
    }
}

bool MacroViewGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if ((keysDown & HidNpadButton_Minus) && m_deleteItem) {
        if (getFocusedElement() == m_deleteItem) {
            ult::deleteFileOrDirectory(m_macroFilePath);
            if (removeHotkeyIfExists(m_macroFilePath, m_gameCfgPath)) g_ipcManager.sendReloadMacroCommand();
            Refresh::RefrRequest(Refresh::MacroGameList);
            tsl::goBack();
            return true;
        }
    } 

    return false;
}



// 快捷键设置
MacroHotKeySettingGui::MacroHotKeySettingGui(const char* macroFilePath, const char* gameName, const char* gameCfgPath, u64 Hotkey) 
 : m_Hotkey(Hotkey), m_selectedButtons(Hotkey)
{
    strcpy(m_macroFilePath, macroFilePath);
    strcpy(m_gameName, gameName);
    strcpy(m_gameCfgPath, gameCfgPath);

    // 读取所有已使用的快捷键（排除当前脚本）
    m_usedHotkeys.clear();
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, m_gameCfgPath);
    for (int i = 1; i <= macroCount; i++) {
        std::string pathKey = "macro_path_" + std::to_string(i);
        std::string comboKey = "macro_combo_" + std::to_string(i);
        std::string path = IniHelper::getString("MACRO", pathKey, "", m_gameCfgPath);
        if (path == macroFilePath) continue;
        u64 combo = static_cast<u64>(IniHelper::getInt("MACRO", comboKey, 0, m_gameCfgPath));
        if (combo != 0) m_usedHotkeys.push_back(combo);
    }
}

tsl::elm::Element* MacroHotKeySettingGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("设置快捷键", m_gameName);
    auto list = new tsl::elm::List();
    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        tsl::Color selectedColor = isHotkeyValid() ? tsl::style::color::ColorHighlight : tsl::Color(0xF, 0x5, 0x5, 0xF);
        if (m_editMode) {   // 编辑模式
            s32 rowH = h / 4; 
            s32 cellW = w / 8;
            const char* text1 = "可选1~3个按键作为组合键";
            auto [t1W, t1H] = r->drawString(text1, false, 0, 0, 18, tsl::Color(0,0,0,0));
            r->drawString(text1, false, x, y + (rowH - t1H) / 2, 18, r->a(tsl::style::color::ColorHighlight));
            const char* text2 = " 选中/取消  退出编辑";
            auto [t2W, t2H] = r->drawString(text2, false, 0, 0, 18, tsl::Color(0,0,0,0));
            r->drawString(text2, false, x, y + rowH + (rowH - t2H) / 2 - 5, 18, r->a(tsl::style::color::ColorHighlight));
            u64 currentTime_ns = armTicksToNs(armGetSystemTick());  // 计算闪烁进度
            double progress = (std::cos(2.0 * 3.14159265358979323846 * std::fmod(currentTime_ns * 0.000000001 - 0.25, 1.0)) + 1.0) * 0.5;
            tsl::Color highlightColor = tsl::Color(
                static_cast<u8>(tsl::highlightColor2.r + (tsl::highlightColor1.r - tsl::highlightColor2.r) * progress),
                static_cast<u8>(tsl::highlightColor2.g + (tsl::highlightColor1.g - tsl::highlightColor2.g) * progress),
                static_cast<u8>(tsl::highlightColor2.b + (tsl::highlightColor1.b - tsl::highlightColor2.b) * progress), 0xF);
            for (int i = 0; i < 16; i++) {
                int row = i / 8;
                int col = i % 8;
                s32 cellX = x + col * cellW;
                s32 cellY = y + rowH * (2 + row);
                u64 btnMask = buttons[i];
                bool isSelected = (m_selectedButtons & btnMask) != 0;
                if (m_cursorIndex == i) r->drawBorderedRoundedRect(cellX - 1, cellY - 8, cellW + 5, 30 + 6, 3, 2, r->a(highlightColor));
                const char* icon = HidHelper::getIconByMask(btnMask);
                auto [iconW, iconH] = r->drawString(icon, false, 0, 0, 30, tsl::Color(0,0,0,0));
                tsl::Color iconColor = isSelected ? selectedColor : tsl::defaultTextColor;
                r->drawString(icon, false, cellX + (cellW - iconW) / 2, cellY + (rowH - iconH) / 2, 30, r->a(iconColor));
            }
        } else {    // 非编辑模式
            if (m_selectedButtons != 0) m_displayText = HidHelper::getCombinedIcons(m_selectedButtons, " + ");  
            else m_displayText = "未设置快捷键";
            s32 fontSize = 32;
            auto [textW, textH] = r->drawString(m_displayText.c_str(), false, 0, 0, fontSize, tsl::Color(0,0,0,0));
            s32 centerX = x + (w - textW) / 2;
            s32 centerY = y + (h - textH) / 2 + textH - 10;
            r->drawStringWithColoredSections(m_displayText.c_str(), false, {" + "}, centerX, centerY, fontSize, selectedColor, tsl::defaultTextColor);
        }
    });
    list->addItem(textArea, 290);

    
    std::string settingValue = m_selectedButtons ? HidHelper::getCombinedIcons(m_selectedButtons, "+") : "未设置快捷键";
    m_HotKeySetting = new tsl::elm::ListItem("设置按键", settingValue);
    list->addItem(m_HotKeySetting);

    bool isSaveValid = isHotkeyValid();
    const char* saveText = isSaveValid ? "按  保存" : "按键不合法";
    m_HotKeySave = new tsl::elm::ListItem("保存按键", saveText);
    m_HotKeySave->setValueColor(isSaveValid ? tsl::style::color::ColorHighlight : tsl::Color(0xF, 0x5, 0x5, 0xF));
    list->addItem(m_HotKeySave);
    m_HotKeyDelete = new tsl::elm::ListItem("删除按键", "按  删除");
    list->addItem(m_HotKeyDelete);
    
    frame->setContent(list);
    return frame;
}

void MacroHotKeySettingGui::update() {
    // 更新设置快捷键和保存按键的value
    if (Refresh::RefrConsume(Refresh::MacroHotKey)) {
        if (m_HotKeySetting) {
            std::string val = m_selectedButtons ? HidHelper::getCombinedIcons(m_selectedButtons, "+") : "未设置快捷键";
            m_HotKeySetting->setValue(val);
        }
        if (m_HotKeySave) {    
            bool isSaveValid = isHotkeyValid();
            const char* saveText = isSaveValid ? "按  保存" : "按键不合法";
            m_HotKeySave->setValue(saveText);
            m_HotKeySave->setValueColor(isSaveValid ? tsl::style::color::ColorHighlight : tsl::Color(0xF, 0x5, 0x5, 0xF));
        }
    }
}

// 检查快捷键是否合法
bool MacroHotKeySettingGui::isHotkeyValid() const {
    if (m_selectedButtons == 0) return false;
    if (m_selectedButtons == tsl::cfg::launchCombo) return false;
    for (u64 used : m_usedHotkeys) if (used == m_selectedButtons) return false;
    // 检查选中按键数量（最多3个）
    int count = 0;
    for (u64 btn : buttons) if (m_selectedButtons & btn) count++;
    if (count > 3) return false;
    return true;
}

bool MacroHotKeySettingGui::handleEditInput(u64 keysDown) {
    if (keysDown & HidNpadButton_AnyLeft) {
        m_cursorIndex = (m_cursorIndex % 8 == 0) ? m_cursorIndex + 7 : m_cursorIndex - 1;
        return true;
    }
    if (keysDown & HidNpadButton_AnyRight) {
        m_cursorIndex = (m_cursorIndex % 8 == 7) ? m_cursorIndex - 7 : m_cursorIndex + 1;
        return true;
    }
    if (keysDown & HidNpadButton_AnyUp) {
        m_cursorIndex = (m_cursorIndex + 8) % 16;
        return true;
    }
    if (keysDown & HidNpadButton_AnyDown) {
        m_cursorIndex = (m_cursorIndex + 8) % 16;
        return true;
    }
    // A键切换选中
    if (keysDown & HidNpadButton_A) {
        u64 btnMask = buttons[m_cursorIndex];
        if (m_selectedButtons & btnMask) m_selectedButtons &= ~btnMask;
        else {
            int count = 0;
            for (u64 btn : buttons) if (m_selectedButtons & btn) count++;
            if (count < 3) m_selectedButtons |= btnMask;
        }
        Refresh::RefrRequest(Refresh::MacroHotKey);
        return true;
    }
    // B键退出编辑模式
    if (keysDown & HidNpadButton_B) {
        m_editMode = false;
        return true;
    }
    return true;
}


bool MacroHotKeySettingGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    // 点击设置按键进入编辑模式
    if (!m_editMode && (keysDown & HidNpadButton_A) && getFocusedElement() == m_HotKeySetting) {
        m_editMode = true;
        return true;
    }
    
    // 编辑模式下的输入处理
    if (m_editMode) return handleEditInput(keysDown);
    
    // 保存按键触发
    if ((keysDown & HidNpadButton_Plus) && m_HotKeySave) {
        if (getFocusedElement() == m_HotKeySave) {
            if (!isHotkeyValid()) return true;
            removeHotkeyIfExists(m_macroFilePath, m_gameCfgPath);
            int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, m_gameCfgPath) + 1;
            IniHelper::setInt("MACRO", "macroCount", macroCount, m_gameCfgPath);
            IniHelper::setString("MACRO", "macro_path_" + std::to_string(macroCount), m_macroFilePath, m_gameCfgPath);
            IniHelper::setInt("MACRO", "macro_combo_" + std::to_string(macroCount), m_selectedButtons, m_gameCfgPath);
            g_ipcManager.sendReloadMacroCommand();
            Refresh::RefrSetMultiple(Refresh::MacroGameList | Refresh::MacroView);
            tsl::goBack();
            return true;
        }
    }
    
    // 删除按键触发
    else if ((keysDown & HidNpadButton_Minus) && m_HotKeyDelete) {
        if (getFocusedElement() == m_HotKeyDelete) {
            if (removeHotkeyIfExists(m_macroFilePath, m_gameCfgPath)) {
                g_ipcManager.sendReloadMacroCommand();
                Refresh::RefrSetMultiple(Refresh::MacroGameList | Refresh::MacroView);
            }
            tsl::goBack();
            return true;
        }
    }
    
    return false;
}





