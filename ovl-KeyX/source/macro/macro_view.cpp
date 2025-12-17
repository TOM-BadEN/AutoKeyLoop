#include "macro_view.hpp"
#include "macro_rename.hpp"
#include "macro_list.hpp"
#include "macro_record.hpp" 
#include "macro_data.hpp"
#include <ultra.hpp>
#include "hiddata.hpp"
#include "refresh.hpp"
#include <algorithm>
#include "ipc.hpp"
#include "macro_edit.hpp"
#include "i18n.hpp"

namespace {
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
    strcpy(m_gameName, gameName);
    MacroData::load(macroFilePath);
    getHotkey();
}

void MacroViewGui::getHotkey() {
    u64 titleId = MacroData::getBasicInfo().titleId;
    m_Hotkey = MacroUtil::getHotkey(titleId, m_macroFilePath);
}


tsl::elm::Element* MacroViewGui::createUI() {
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    frame->setHeader(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString(m_gameName, false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString("管理录制的脚本", false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
        renderer->drawString("  脚本介绍", false, 270, 693, 23, renderer->a(tsl::style::color::ColorText));
    }));
    auto list = new tsl::elm::List();
    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        const auto& info = MacroData::getBasicInfo();
        char titleId[17], fileSize[16], duration[16], fps[16], frames[16], fileName[80];
        snprintf(titleId, sizeof(titleId), "%016lX", info.titleId);
        snprintf(fileName, sizeof(fileName), "%s", info.fileName);
        snprintf(fileSize, sizeof(fileSize), "%u KB (V%u)", (info.fileSize + 1023) / 1024, info.version);
        snprintf(duration, sizeof(duration), "%u s", info.durationMs / 1000);
        snprintf(fps, sizeof(fps), "%u FPS", info.frameRate);
        snprintf(frames, sizeof(frames), "%u", info.frameCount);
        
        constexpr const char* kLabels[] = {
            "游戏编号：", "脚本名称：",
            "脚本大小：", "录制时间：", "录制帧率：", "录制帧数："
        };
        const char* values[] = { titleId, fileName, fileSize, duration, fps, frames };
        
        char line[64];
        constexpr const u32 fontSize = 20;
        const s32 startX = x + 26;
        s32 lineHeight = r->getTextDimensions("你", false, fontSize).second + 15;
        s32 totalHeight = lineHeight * static_cast<s32>(std::size(kLabels));
        s32 startY = y + (h - totalHeight) / 2 - 20;
        for (size_t i = 0; i < std::size(kLabels); ++i) {
            snprintf(line, sizeof(line), "%s%s", i18n(kLabels[i]).c_str(), values[i]);
            r->drawStringWithColoredSections(
                line,
                false,
                {i18n(kLabels[i])},
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
            tsl::changeTo<MacroHotKeySettingGui>(m_macroFilePath, m_gameName, m_Hotkey);
            return true;
        }
        return false;
    });
    list->addItem(m_listButton);

    auto listChangeName = new tsl::elm::ListItem("修改名称", ">");
    listChangeName->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<MacroRenameGui>(m_macroFilePath, m_gameName);
            return true;
        }   
        return false;
    });
    list->addItem(listChangeName);

    auto listEditMacro = new tsl::elm::ListItem("编辑脚本", ">");
    listEditMacro->setClickListener([this](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<MacroEditGui>(m_gameName, m_isRecord);
            return true;
        }   
        return false;
    });  
    list->addItem(listEditMacro);


    m_deleteItem = new tsl::elm::ListItem("删除脚本","按  删除");
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
    if (keysDown & HidNpadButton_B) {
        MacroData::allCleanup();
        tsl::goBack();
        return true;
    }
    if (keysDown & HidNpadButton_Right) {
        tsl::changeTo<MacroDescGui>(m_macroFilePath, m_gameName);
        return true;
    }
    if ((keysDown & HidNpadButton_Minus) && m_deleteItem) {
        if (getFocusedElement() == m_deleteItem) {
            u64 titleId = MacroData::getBasicInfo().titleId;
            if (MacroUtil::deleteMacro(titleId, m_macroFilePath)) g_ipcManager.sendReloadMacroCommand();
            Refresh::RefrRequest(Refresh::MacroGameList);
            MacroData::allCleanup();
            tsl::goBack();
            return true;
        }
    } 

    return false;
}



// 快捷键设置
MacroHotKeySettingGui::MacroHotKeySettingGui(const char* macroFilePath, const char* gameName, u64 Hotkey) 
 : m_Hotkey(Hotkey), m_selectedButtons(Hotkey)
{
    strcpy(m_macroFilePath, macroFilePath);
    strcpy(m_gameName, gameName);
    m_titleId = MacroUtil::getTitleIdFromPath(macroFilePath);
    // 读取所有已使用的快捷键
    m_usedHotkeys = MacroUtil::getUsedHotkeys(m_titleId);
    // 移除当前脚本的快捷键（如果存在）
    auto it = std::find(m_usedHotkeys.begin(), m_usedHotkeys.end(), Hotkey);
    if (it != m_usedHotkeys.end()) m_usedHotkeys.erase(it);
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
    const char* saveText = isSaveValid ? "按  保存" : "按键不合法";
    m_HotKeySave = new tsl::elm::ListItem("保存按键", saveText);
    m_HotKeySave->setValueColor(isSaveValid ? tsl::style::color::ColorHighlight : tsl::Color(0xF, 0x5, 0x5, 0xF));
    list->addItem(m_HotKeySave);
    m_HotKeyDelete = new tsl::elm::ListItem("删除按键", "按  删除");
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
            const char* saveText = isSaveValid ? "按  保存" : "按键不合法";
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
            MacroUtil::setHotkey(m_titleId, m_macroFilePath, m_selectedButtons);
            g_ipcManager.sendReloadMacroCommand();
            Refresh::RefrSetMultiple(Refresh::MacroGameList | Refresh::MacroView);
            tsl::goBack();
            return true;
        }
    }
    
    // 删除按键触发
    else if ((keysDown & HidNpadButton_Minus) && m_HotKeyDelete) {
        if (getFocusedElement() == m_HotKeyDelete) {
            if (MacroUtil::removeHotkey(m_titleId, m_macroFilePath)) {
                g_ipcManager.sendReloadMacroCommand();
                Refresh::RefrSetMultiple(Refresh::MacroGameList | Refresh::MacroView);
            }
            tsl::goBack();
            return true;
        }
    }
    
    return false;
}


// 使用说明界面
MacroDescGui::MacroDescGui(const char* macroFilePath, const char* gameName) {
    strcpy(m_gameName, gameName);
    strcpy(m_filePath, macroFilePath);
    m_meta = MacroUtil::getMetadata(macroFilePath);
}

tsl::elm::Element* MacroDescGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame(m_gameName, "阅读脚本的相关介绍");
    
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        s32 textX = x + 19;
        s32 currentY = y + 35;
        
        // 脚本名称 + 作者（没有name则显示文件名，去掉.macro后缀）
        std::string displayName = m_meta.name;
        if (displayName.empty()) {
            displayName = ult::getFileName(m_filePath);
            if (displayName.size() > 6) displayName = displayName.substr(0, displayName.size() - 6);
        }
        auto nameDim = r->getTextDimensions(displayName, false, 28);
        r->drawString(displayName, false, textX, currentY, 28, r->a(tsl::onTextColor));
        if (!m_meta.author.empty()) {
            std::string byText = " by " + m_meta.author;
            r->drawString(byText, false, textX + nameDim.first, currentY, 20, r->a(tsl::onTextColor));
        }
        currentY += 50;
        
        // 通用换行绘制函数
        s32 maxWidth = w - 19 - 15;
        s32 fontSize = 18;
        s32 lineHeight = 26;
        s32 maxY = y + h - 20;
        
        auto drawWrappedText = [&](const std::string& content, const std::string& prefix) {
            std::string text = content;
            auto [prefixW, prefixH] = r->getTextDimensions(prefix, false, fontSize);
            s32 drawX = textX;
            bool isFirstLine = true;
            
            while (!text.empty() && currentY <= maxY) {
                s32 lineMaxWidth = isFirstLine ? maxWidth : (maxWidth - prefixW);
                auto [tw, th] = r->getTextDimensions(text, false, fontSize);
                
                if (tw <= lineMaxWidth) {
                    if (isFirstLine) {
                        r->drawString(prefix + text, false, drawX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
                    } else {
                        r->drawString(text, false, drawX + prefixW, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
                    }
                    currentY += lineHeight;
                    break;
                }
                
                // 获取 UTF-8 字符边界
                std::vector<size_t> charBounds = {0};
                for (size_t i = 0; i < text.size(); ) {
                    unsigned char c = text[i];
                    if ((c & 0x80) == 0) i += 1;
                    else if ((c & 0xE0) == 0xC0) i += 2;
                    else if ((c & 0xF0) == 0xE0) i += 3;
                    else i += 4;
                    charBounds.push_back(i);
                }
                
                // 二分查找截断点
                size_t lo = 1, hi = charBounds.size() - 1, cutIdx = 1;
                while (lo <= hi) {
                    size_t mid = (lo + hi) / 2;
                    auto [mw, mh] = r->getTextDimensions(text.substr(0, charBounds[mid]), false, fontSize);
                    if (mw <= lineMaxWidth) {
                        cutIdx = mid;
                        lo = mid + 1;
                    } else {
                        hi = mid - 1;
                    }
                }
                
                size_t cut = charBounds[cutIdx];
                if (isFirstLine) {
                    r->drawString(prefix + text.substr(0, cut), false, drawX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
                    isFirstLine = false;
                } else {
                    r->drawString(text.substr(0, cut), false, drawX + prefixW, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
                }
                currentY += lineHeight;
                text = text.substr(cut);
            }
        };
        
        // 文件位置
        r->drawString("文件位置:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
        currentY += 28;
        drawWrappedText(m_filePath, " • ");
        currentY += 20;
        
        // 使用说明
        if (m_meta.desc.empty()) {
            r->drawString("使用说明:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
            currentY += 28;
            r->drawString(" • 暂无使用说明", false, textX, currentY, 18, r->a(tsl::style::color::ColorDescription));
        } else {
            r->drawString("使用说明:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
            currentY += 28;
            
            // 按 \n 分割描述文本
            std::string desc = m_meta.desc;
            size_t pos = 0;
            while ((pos = desc.find('\n')) != std::string::npos) {
                drawWrappedText(desc.substr(0, pos), " • ");
                desc = desc.substr(pos + 1);
            }
            if (!desc.empty()) {
                drawWrappedText(desc, " • ");
            }
        }
    });
    
    frame->setContent(drawer);
    return frame;
}

bool MacroDescGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_Left || keysDown & HidNpadButton_B) {
        tsl::goBack();
        return true;
    }
    return false;
}
