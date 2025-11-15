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
    list->addItem(textArea, 310);

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
    } else if (m_isRecord && (keysDown & HidNpadButton_B)) {
        tsl::goBack(5); // 返回到脚本设置界面
        return true;
    }

    return false;
}

// 快捷键设置
MacroHotKeySettingGui::MacroHotKeySettingGui(const char* macroFilePath, const char* gameName, const char* gameCfgPath, u64 Hotkey) 
 : m_Hotkey(Hotkey)
{
    strcpy(m_macroFilePath, macroFilePath);
    strcpy(m_gameName, gameName);
    strcpy(m_gameCfgPath, gameCfgPath);
    // 读取所有已使用的快捷键（排除当前脚本）
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, m_gameCfgPath);
    for (int i = 1; i <= macroCount; i++) {
        std::string pathKey = "macro_path_" + std::to_string(i);
        std::string comboKey = "macro_combo_" + std::to_string(i);
        std::string path = IniHelper::getString("MACRO", pathKey, "", m_gameCfgPath);
        if (path == macroFilePath) continue;  // 跳过当前脚本
        u64 combo = static_cast<u64>(IniHelper::getInt("MACRO", comboKey, 0, m_gameCfgPath));
        if (combo != 0) m_usedHotkeys.push_back(combo);
    }
}

tsl::elm::Element* MacroHotKeySettingGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("设置快捷键", m_gameName);
    auto list = new tsl::elm::List();
    list->addItem(new tsl::elm::CategoryHeader(" 选择触发脚本的快捷键"));
    // 添加"删除快捷键"选项
    auto deleteItem = new tsl::elm::ListItem("删除快捷键");
    deleteItem->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            if (removeHotkeyIfExists(m_macroFilePath, m_gameCfgPath)) {
                g_ipcManager.sendReloadMacroCommand();
                Refresh::RefrSetMultiple(Refresh::MacroGameList | Refresh::MacroView);
            }
            tsl::goBack();
            return true;
        }
        return false;
    });
    list->addItem(deleteItem);

    for (const u64 combo : MacroHotkeys::Hotkeys) {
        if (combo == tsl::cfg::launchCombo) continue;  // 跳过特斯拉快捷键
        // 跳过已被其他脚本使用的快捷键
        if (std::find(m_usedHotkeys.begin(), m_usedHotkeys.end(), combo) != m_usedHotkeys.end()) continue;
        std::string iconStr = HidHelper::getCombinedIcons(combo);
        auto listSetHotKey = new tsl::elm::ListItem(iconStr);
        if (combo == m_Hotkey) listSetHotKey->setValue("");
        listSetHotKey->setClickListener([this, combo](u64 keys) {
            if (keys & HidNpadButton_A) {
                removeHotkeyIfExists(m_macroFilePath, m_gameCfgPath);
                int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, m_gameCfgPath) + 1;
                IniHelper::setInt("MACRO", "macroCount", macroCount, m_gameCfgPath);
                IniHelper::setString("MACRO", "macro_path_" + std::to_string(macroCount), m_macroFilePath, m_gameCfgPath);
                IniHelper::setInt("MACRO", "macro_combo_" + std::to_string(macroCount), combo, m_gameCfgPath);
                g_ipcManager.sendReloadMacroCommand();
                Refresh::RefrSetMultiple(Refresh::MacroGameList | Refresh::MacroView);
                tsl::goBack();
                return true;
            } 
            return false;
        });
        list->addItem(listSetHotKey);
    }

    // 如果已有快捷键，跳转到对应的项
    if (m_Hotkey != 0) {
        std::string currentIconStr = HidHelper::getCombinedIcons(m_Hotkey);
        list->jumpToItem(currentIconStr, "");
    }

    frame->setContent(list);
    return frame;
}




