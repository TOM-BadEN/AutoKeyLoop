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

    struct MacroHotkeySlots {
        u64 slot1 = 0;  // 一号按键
        u64 slot2 = 0;  // 二号按键
        u64 slot3 = 0;  // 三号按键
    };

    MacroHotkeySlots s_hotkeySlots{};
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

    s_hotkeySlots = {};
    int slotIndex = 0;
    for (u64 btn : buttons) {
        if (!(m_Hotkey & btn)) continue;
        if (slotIndex == 0) s_hotkeySlots.slot1 = btn;
        else if (slotIndex == 1) s_hotkeySlots.slot2 = btn;
        else if (slotIndex == 2) s_hotkeySlots.slot3 = btn;
        slotIndex++;
        if (slotIndex >= 3) break;
    }
    // 读取所有已使用的快捷键（排除当前脚本）
    m_usedHotkeys.clear();
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
    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        u64 combo = s_hotkeySlots.slot1 | s_hotkeySlots.slot2 | s_hotkeySlots.slot3;
        if (combo != 0) m_displayText = HidHelper::getCombinedIcons(combo, " + ");  
        else m_displayText = "未设置快捷键";
        s32 fontSize = 32;
        auto textDim = r->getTextDimensions(m_displayText, false, fontSize);
        s32 textWidth = textDim.first;
        s32 textHeight = textDim.second;
        s32 centerX = x + (w - textWidth) / 2;
        s32 centerY = y + (h - textHeight) / 2 + textHeight - 10;
        
        // 根据快捷键是否合法选择颜色(合法=青色，不合法=红色)
        tsl::Color iconColor = isHotkeyValid() ? tsl::style::color::ColorHighlight : tsl::Color(0xF, 0x5, 0x5, 0xF);  
        
        r->drawStringWithColoredSections(
            m_displayText,              
            false,
            {" + "},                          
            centerX, centerY, fontSize,
            iconColor,             
            tsl::defaultTextColor             
        );

    });
    list->addItem(textArea, 150);


    m_HotKeyNO1 = new tsl::elm::ListItem("一号按键", s_hotkeySlots.slot1 ? HidHelper::getIconByMask(s_hotkeySlots.slot1) : "无");
    m_HotKeyNO1->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<ButtonSelectGui>(1);
            return true;
        }
        return false;
    });
    list->addItem(m_HotKeyNO1);
    
    m_HotKeyNO2 = new tsl::elm::ListItem("二号按键", s_hotkeySlots.slot2 ? HidHelper::getIconByMask(s_hotkeySlots.slot2) : "无");
    m_HotKeyNO2->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            if (s_hotkeySlots.slot1 > 0) tsl::changeTo<ButtonSelectGui>(2);
            return true;
        }
        return false;
    });
    list->addItem(m_HotKeyNO2);

    m_HotKeyNO3 = new tsl::elm::ListItem("三号按键", s_hotkeySlots.slot3 ? HidHelper::getIconByMask(s_hotkeySlots.slot3) : "无");
    m_HotKeyNO3->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            if (s_hotkeySlots.slot2 > 0) tsl::changeTo<ButtonSelectGui>(3);
            return true;
        }
        return false;
    });
    list->addItem(m_HotKeyNO3);

    bool isSaveValid = isHotkeyValid();
    const char* saveText = isSaveValid ? "按  保存" : "按键不合法";
    m_HotKeySave = new tsl::elm::ListItem("保存按键", saveText);
    m_HotKeySave->setValueColor(isSaveValid ? tsl::style::color::ColorHighlight : tsl::Color(0xF, 0x5, 0x5, 0xF));
    list->addItem(m_HotKeySave);
    m_HotKeyDelete = new tsl::elm::ListItem("删除按键", "按  删除");
    list->addItem(m_HotKeyDelete);
    
    
    frame->setContent(list);
    return frame;
}

void MacroHotKeySettingGui::update() {
    if (Refresh::RefrConsume(Refresh::MacroHotKey)) {
        m_HotKeyNO1->setValue(s_hotkeySlots.slot1 ? HidHelper::getIconByMask(s_hotkeySlots.slot1) : "无");
        m_HotKeyNO2->setValue(s_hotkeySlots.slot2 ? HidHelper::getIconByMask(s_hotkeySlots.slot2) : "无");
        m_HotKeyNO3->setValue(s_hotkeySlots.slot3 ? HidHelper::getIconByMask(s_hotkeySlots.slot3) : "无");
        bool isSaveValid = isHotkeyValid();
        const char* saveText = isSaveValid ? "按  保存" : "按键不合法";
        m_HotKeySave->setValue(saveText);
        m_HotKeySave->setValueColor(isSaveValid ? tsl::style::color::ColorHighlight : tsl::Color(0xF, 0x5, 0x5, 0xF));
    }
}

// 检查快捷键是否合法
// 1. 至少有一个按键
// 2. 组合不能与其他脚本冲突
// 3. 组合不能与特斯拉快捷键冲突
bool MacroHotKeySettingGui::isHotkeyValid() const {
    u64 combo = s_hotkeySlots.slot1 | s_hotkeySlots.slot2 | s_hotkeySlots.slot3;
    if (combo == 0) return false;
    for (u64 used : m_usedHotkeys) if (used == combo) return false;
    if (combo == tsl::cfg::launchCombo) return false;
    return true;
}

bool MacroHotKeySettingGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    // 保存按键触发
    if ((keysDown & HidNpadButton_Plus) && m_HotKeySave) {
        if (getFocusedElement() == m_HotKeySave) {
            if (!isHotkeyValid()) return true;
            removeHotkeyIfExists(m_macroFilePath, m_gameCfgPath);
            u64 combo = s_hotkeySlots.slot1 | s_hotkeySlots.slot2 | s_hotkeySlots.slot3;
            int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, m_gameCfgPath) + 1;
            IniHelper::setInt("MACRO", "macroCount", macroCount, m_gameCfgPath);
            IniHelper::setString("MACRO", "macro_path_" + std::to_string(macroCount), m_macroFilePath, m_gameCfgPath);
            IniHelper::setInt("MACRO", "macro_combo_" + std::to_string(macroCount), combo, m_gameCfgPath);
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



// 按键选择类
ButtonSelectGui::ButtonSelectGui(int slotIndex) 
    : m_slotIndex(slotIndex) 
{
    // 判断是否可以删除
    if (m_slotIndex == 1) m_canDelete = (s_hotkeySlots.slot2 == 0);
    else if (m_slotIndex == 2) m_canDelete = (s_hotkeySlots.slot3 == 0);
    else if (m_slotIndex == 3) m_canDelete = true;
}

tsl::elm::Element* ButtonSelectGui::createUI() {
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    frame->setHeader(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        r->drawString("选择按键", false, 20, 52, 32, r->a(tsl::defaultOverlayColor));
        r->drawString("选择第" + std::to_string(m_slotIndex) + "号按键", false, 20, 73, 15, r->a(tsl::bannerVersionTextColor));
        if (m_canDelete) r->drawString("\uE0EE  删除", false, 280, 693, 23, r->a(tsl::style::color::ColorText));
    }));
    auto list = new tsl::elm::List();
    list->addItem(new tsl::elm::CategoryHeader(" 选择要使用的快捷键"));

    // 获取当前槽位的值
    u64 currentSlotValue = 0;
    if (m_slotIndex == 1) currentSlotValue = s_hotkeySlots.slot1;
    else if (m_slotIndex == 2) currentSlotValue = s_hotkeySlots.slot2;
    else if (m_slotIndex == 3) currentSlotValue = s_hotkeySlots.slot3;
    
    // 遍历所有可选按键
    for (u64 btn : buttons) {
        // 过滤掉其他两个槽位已使用的按键
        if (m_slotIndex == 1 && (btn == s_hotkeySlots.slot2 || btn == s_hotkeySlots.slot3)) continue;
        else if (m_slotIndex == 2 && (btn == s_hotkeySlots.slot1 || btn == s_hotkeySlots.slot3)) continue;
        else if (m_slotIndex == 3 && (btn == s_hotkeySlots.slot1 || btn == s_hotkeySlots.slot2)) continue;
        
        // 判断是否是当前选中
        bool isCurrent = (currentSlotValue == btn);
        auto item = new tsl::elm::ListItem(
            ult::i18n("按键  ") + HidHelper::getIconByMask(btn),
            isCurrent ? ult::i18n("当前") : ""
        );
        
        item->setClickListener([this, btn](u64 keys) {
            if (keys & HidNpadButton_A) {
                if (m_slotIndex == 1) s_hotkeySlots.slot1 = btn;
                else if (m_slotIndex == 2) s_hotkeySlots.slot2 = btn;
                else if (m_slotIndex == 3) s_hotkeySlots.slot3 = btn;
                Refresh::RefrRequest(Refresh::MacroHotKey);
                tsl::goBack();
                return true;
            }
            return false;
        });
        list->addItem(item);
    }
    
    // 跳转到当前选中项
    list->jumpToItem("", ult::i18n("当前"));
    
    frame->setContent(list);
    return frame;
}

bool ButtonSelectGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (m_canDelete && (keysDown & HidNpadButton_Right)) {
        if (m_slotIndex == 1) s_hotkeySlots.slot1 = 0;
        else if (m_slotIndex == 2) s_hotkeySlots.slot2 = 0;
        else if (m_slotIndex == 3) s_hotkeySlots.slot3 = 0;
        Refresh::RefrRequest(Refresh::MacroHotKey);
        tsl::goBack();
        return true;
    }
    return false;
}



