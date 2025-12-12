#include "macro_edit.hpp"
#include "macro_data.hpp"
#include "hiddata.hpp"
#include <ultra.hpp>
#include "refresh.hpp"
#include "ipc.hpp"
#include "macro_view.hpp"

namespace {
    static constexpr s32 TIMELINE_HEIGHT = 100;       // 时间轴区域高度
    static constexpr s32 ITEM_HEIGHT = 70;            // 每个动作项的高度
    static constexpr u32 REPEAT_DELAY = 20;           // 首次触发后延迟帧数
    static constexpr u32 REPEAT_RATE = 8;             // 重复触发间隔帧数

    // 摇杆方向图标
    const char* StickDirIcons[] = {"", "", "", "", "", "", "", "", ""};
    const char* getStickDirIcon(StickDir dir) { return StickDirIcons[static_cast<int>(dir)]; }
    
    // 生成动作显示文本
    std::string getActionText(const Action& action) {
        std::string text;
        if (action.stickL != StickDir::None) text = text + "" + getStickDirIcon(action.stickL);
        if (action.stickR != StickDir::None) text = text + (!text.empty() ? "+" : "") + "" + getStickDirIcon(action.stickR);
        if (action.buttons != 0) text = text + (!text.empty() ? "+" : "") + HidHelper::getCombinedIcons(action.buttons);
        if (text.empty()) text = "无动作";
        return text;
    }
    
    // 计算闪烁进度 (0.0~1.0)
    double calcBlinkProgress() {
        u64 ns = armTicksToNs(armGetSystemTick());
        return (std::cos(2.0 * 3.14159265358979323846 * std::fmod(ns * 0.000000001 - 0.25, 1.0)) + 1.0) * 0.5;
    }
    
    // 计算闪烁颜色（在两个颜色之间插值）
    tsl::Color calcBlinkColor(tsl::Color c1, tsl::Color c2, double progress) {
        return tsl::Color(
            static_cast<u8>(c2.r + (c1.r - c2.r) * progress),
            static_cast<u8>(c2.g + (c1.g - c2.g) * progress),
            static_cast<u8>(c2.b + (c1.b - c2.b) * progress), 0xF);
    }
    
    // 按键编辑模式的按键布局
    constexpr const u64 EDIT_BTN_MASKS[16] = {
        BTN_A, BTN_B, BTN_X, BTN_Y, BTN_L, BTN_R, BTN_ZL, BTN_ZR,
        BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_SELECT, BTN_START, BTN_STICKL, BTN_STICKR
    };
    
    // 菜单项
    enum MenuItem { InsertBefore, InsertAfter, Reset, Delete, EditDuration, EditButton };
}

MacroEditGui::MacroEditGui(const char* macroFilePath, const char* gameName, bool isRecord) 
 : m_macroFilePath(macroFilePath)
 , m_gameName(gameName)
 , m_isRecord(isRecord)
{
    MacroData::load(macroFilePath);
    MacroData::parseActions();
}

MacroEditGui::~MacroEditGui() {
    MacroData::cleanup();
}

tsl::elm::Element* MacroEditGui::createUI() {
    // 提取文件名（去掉扩展名）
    std::string fileName = ult::getFileName(m_macroFilePath);
    auto dot = fileName.rfind('.');
    if (dot != std::string::npos) fileName = fileName.substr(0, dot);
    std::string subtitle = fileName + " 撤销修改";
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    
    // 自定义头部绘制器（包含标题和底部按钮）
    frame->setHeader(new tsl::elm::CustomDrawer([this, subtitle](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        // 标题（菜单模式下显示菜单项名称）
        if (m_menuMode) {
            const char* menuTitles[] = {"在前面插入新动作", "在后面插入新动作", "重置动作", "删除动作", "修改持续时间", "修改触发按钮"};
            const char* menuTips[] = {"按  插入新动作", "按  插入新动作", "按  重置动作为无动作", "删除整个动作", "/ 切换位 / 调整  保存  取消", " 选中/取消选中  保存  返回"};
            r->drawString(menuTitles[m_menuIndex], false, 20, 50, 32, r->a(tsl::defaultOverlayColor));
            r->drawString(menuTips[m_menuIndex], false, 20, 73, 15, r->a(tsl::style::color::ColorDescription));
        } else {
            r->drawString(m_gameName, false, 20, 50, 32, r->a(tsl::defaultOverlayColor));
            tsl::Color undoColor = MacroData::canUndo() ? r->a(tsl::onTextColor) : tsl::style::color::ColorDescription;
            r->drawStringWithColoredSections(subtitle.c_str(), false, {" 撤销修改"}, 20, 73, 15, tsl::style::color::ColorDescription, undoColor);
        }
        const char* btnText = m_selectMode ? "  修改" : "  保存";
        r->drawString(btnText, false, 280, 693, 23, r->a(tsl::style::color::ColorText));
    }));
    
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        double progress = calcBlinkProgress();
        if (m_buttonEditMode) drawButtonEditArea(r, x, y, w, progress);
        else if (m_durationEditMode) drawDurationEditArea(r, x, y, w, progress);
        else if (m_menuMode) drawMenuArea(r, x, y, w, progress);
        else drawTimelineArea(r, x, y, w, progress);
        drawActionList(r, x, y, w, h, progress);
    });
    
    frame->setContent(drawer);
    return frame;
}

bool MacroEditGui::isMenuItemEnabled(int index) const {
    auto& actions = MacroData::getActions();
    s32 count = m_menuSelEnd - m_menuSelStart + 1;
    
    // "删除动作"(index=3)：选中全部时不可删除
    if (index == 3) {
        return count < (s32)actions.size();
    }
    
    // "修改持续时间"(index=4)和"修改触发按钮"(index=5)需要判断
    if (index != 4 && index != 5) return true;
    
    if (count > 1) return false;
    
    // 检查是否是"仅摇杆"（有摇杆 && 无按键）
    const auto& action = actions[m_menuSelStart];
    bool isStickOnly = (action.buttons == 0) && 
                       (action.stickL != StickDir::None || action.stickR != StickDir::None);
    return !isStickOnly;
}

void MacroEditGui::drawActionList(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h, double progress) {
    auto& actions = MacroData::getActions();
    auto& header = MacroData::getHeader();
    
    // 顶部分隔线
    r->drawRect(x, y + 100 - 1, w, 1, r->a(tsl::separatorColor));
    
    s32 listY = y + 100 + 5;
    s32 listH = h - 100 - 73;
    m_listHeight = listH;
    s32 bottomY = y + h - 73;
    
    for (size_t i = 0; i < actions.size(); i++) {
        s32 itemY = listY + i * ITEM_HEIGHT - m_scrollOffset;
        if (itemY + ITEM_HEIGHT > listY && itemY < bottomY) {
            r->drawRect(x + 4, itemY + ITEM_HEIGHT - 1, w - 8, 1, r->a(tsl::separatorColor));
            
            if ((s32)i == m_selectedIndex) {
                tsl::Color hlColor = m_selectMode 
                    ? calcBlinkColor(tsl::Color(0xF,0xD,0x0,0xF), tsl::Color(0x8,0x6,0x0,0xF), progress)
                    : calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
                r->drawBorderedRoundedRect(x, itemY, w + 4, ITEM_HEIGHT, 5, 5, r->a(hlColor));
            }
            
            std::string text = getActionText(actions[i]);
            u32 durationMs = actions[i].frameCount * 1000 / header.frameRate;
            std::string duration = std::to_string(durationMs) + "ms";
            std::string fullText = std::to_string(i + 1) + ". " + text;
            
            bool inSelectRange = false;
            if (m_selectMode && m_selectAnchor >= 0) {
                s32 selStart = std::min(m_selectAnchor, m_selectedIndex);
                s32 selEnd = std::max(m_selectAnchor, m_selectedIndex);
                inSelectRange = ((s32)i >= selStart && (s32)i <= selEnd);
            }
            tsl::Color textColor = inSelectRange ? tsl::Color(0xF, 0xD, 0x0, 0xF) 
                                 : (actions[i].modified ? tsl::Color(0xF, 0xA, 0xC, 0xF) : tsl::defaultTextColor);
            
            auto [durW, durH] = r->drawString(duration.c_str(), false, 0, 0, 20, tsl::Color(0,0,0,0));
            s32 maxTextWidth = w - 19 - durW - 25;
            std::string truncatedText = r->limitStringLength(fullText, false, 23, maxTextWidth);
            r->drawString(truncatedText.c_str(), false, x + 19, itemY + 45, 23, r->a(textColor));
            r->drawString(duration.c_str(), false, x + w - 15 - durW, itemY + 45, 20, r->a(tsl::onTextColor));
        }
    }
}

void MacroEditGui::drawTimelineArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress) {
    auto& actions = MacroData::getActions();
    auto& header = MacroData::getHeader();
    float totalDuration = (float)header.frameCount / header.frameRate;
    float pixelPerSec = (float)(w - 20) / totalDuration;
    s32 barH = 25, barY = y + 50, tickY = y + 27;
    
    // 时间刻度
    s32 tickInterval = (totalDuration > 20) ? 5 : (totalDuration > 10) ? 2 : 1;
    for (s32 t = 0; t <= (s32)totalDuration; t += tickInterval) {
        s32 tickX = x + 10 + (s32)(t * pixelPerSec);
        r->drawRect(tickX, tickY + 5, 1, 10, r->a(tsl::Color(0x8, 0x8, 0x8, 0xF)));
        char buf[16]; snprintf(buf, sizeof(buf), "%ds", t);
        r->drawString(buf, false, tickX - 5, tickY, 12, r->a(tsl::Color(0xA, 0xA, 0xA, 0xF)));
    }
    
    // 选择模式高亮色
    tsl::Color timelineHighlight = m_selectMode 
        ? calcBlinkColor(tsl::Color(0xF,0xD,0x0,0xF), tsl::Color(0x8,0x6,0x0,0xF), progress) 
        : calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
    
    // 选择范围
    s32 selStart = m_selectedIndex, selEnd = m_selectedIndex;
    if (m_selectMode && m_selectAnchor >= 0) {
        selStart = std::min(m_selectAnchor, m_selectedIndex);
        selEnd = std::max(m_selectAnchor, m_selectedIndex);
    }
    
    // 动作色块
    float currentTime = 0;
    s32 rangeStartX = -1, rangeEndX = -1;
    for (size_t i = 0; i < actions.size(); i++) {
        float dur = (float)actions[i].frameCount / header.frameRate;
        s32 blockX = x + 10 + (s32)(currentTime * pixelPerSec);
        s32 blockW = std::max(1, (s32)(dur * pixelPerSec));
        bool inRange = ((s32)i >= selStart && (s32)i <= selEnd);
        s32 drawY = inRange ? barY - 3 : barY;
        s32 drawH = inRange ? barH + 6 : barH;
        tsl::Color blockColor = (actions[i].buttons != 0) ? tsl::Color(0x2, 0xA, 0x2, 0xF)
                              : (actions[i].stickL != StickDir::None || actions[i].stickR != StickDir::None) ? tsl::Color(0xA, 0x2, 0x2, 0xF)
                              : tsl::Color(0x8, 0x8, 0x8, 0xF);
        r->drawRect(blockX, drawY, blockW, drawH, r->a(blockColor));
        if ((s32)i == selStart) rangeStartX = blockX;
        if ((s32)i == selEnd) rangeEndX = blockX + blockW;
        currentTime += dur;
    }
    
    // 选中框
    if (rangeStartX >= 0 && rangeEndX >= 0) {
        s32 boxX = rangeStartX - 2, boxW = rangeEndX - rangeStartX + 4, boxY = barY - 3, boxH = barH + 6;
        r->drawRect(boxX, boxY, boxW, 2, r->a(timelineHighlight));
        r->drawRect(boxX, boxY + boxH, boxW, 2, r->a(timelineHighlight));
        r->drawRect(boxX, boxY, 2, boxH, r->a(timelineHighlight));
        r->drawRect(rangeEndX, boxY, 2, boxH, r->a(timelineHighlight));
    }
}

void MacroEditGui::drawMenuArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress) {
    tsl::Color highlightColor = calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
    s32 itemW = w / 6;
    const char* icons[] = {"", "", "", "", "", ""};
    for (int i = 0; i < 6; i++) {
        s32 itemX = x + i * itemW;
        bool enabled = isMenuItemEnabled(i);
        tsl::Color iconColor = enabled ? tsl::defaultTextColor : tsl::Color(0x5, 0x5, 0x5, 0xF);
        auto [iconW, iconH] = r->drawString(icons[i], false, 0, 0, 30, tsl::Color(0,0,0,0));
        r->drawString(icons[i], false, itemX + (itemW - iconW) / 2, y + 58, 30, r->a(iconColor));
        if (i < 5) r->drawRect(itemX + itemW, y + 22, 1, 56, r->a(tsl::separatorColor));
        if (i == m_menuIndex) r->drawBorderedRoundedRect(itemX, y + 7, itemW + 5, 73, 5, 5, r->a(highlightColor));
    }
}

void MacroEditGui::drawDurationEditArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress) {
    tsl::Color highlightColor = calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
    auto& actions = MacroData::getActions();
    auto& header = MacroData::getHeader();
    s32 currentActionMs = actions[m_menuSelStart].frameCount * 1000 / header.frameRate;
    s32 totalMs = header.frameCount * 1000 / header.frameRate;
    s32 maxEditMs = 30000 - (totalMs - currentActionMs);
    bool isOverLimit = m_editingDuration > maxEditMs;
    s32 digits[5] = { m_editingDuration/10000%10, m_editingDuration/1000%10, m_editingDuration/100%10, m_editingDuration/10%10, m_editingDuration%10 };
    s32 digitW = 30, gap = 8;
    s32 totalW = digitW * 5 + gap * 5 + 30;
    s32 startX = x + (w - totalW) / 2;
    tsl::Color errorColor = tsl::Color(0xF, 0x2, 0x2, 0xF);
    tsl::Color errorHighlight = calcBlinkColor(tsl::Color(0xF,0x4,0x4,0xF), tsl::Color(0x8,0x1,0x1,0xF), progress);
    for (int i = 0; i < 5; i++) {
        char buf[2] = {(char)('0' + digits[i]), '\0'};
        tsl::Color color = isOverLimit ? ((i == m_editDigitIndex) ? errorHighlight : errorColor) 
                                       : ((i == m_editDigitIndex) ? highlightColor : tsl::defaultTextColor);
        r->drawString(buf, false, startX + i * (digitW + gap), y + 60, 36, r->a(color));
    }
    r->drawString("ms", false, startX + 5 * (digitW + gap), y + 60, 28, r->a(isOverLimit ? errorColor : tsl::defaultTextColor));
    
    if (isOverLimit) {
        char maxBuf[32];
        snprintf(maxBuf, sizeof(maxBuf), "最大: %dms", maxEditMs);
        auto [maxW, maxH] = r->drawString(maxBuf, false, 0, 0, 14, tsl::Color(0,0,0,0));
        r->drawString(maxBuf, false, x + w - 15 - maxW, y + 90, 14, r->a(errorColor));
    }
}

void MacroEditGui::drawButtonEditArea(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, double progress) {
    tsl::Color highlightColor = calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
    s32 cellW = w / 8;
    s32 rowH = 40;
    tsl::Color staticHighlight = tsl::Color(0x0, 0xF, 0xF, 0xF);
    for (int i = 0; i < 16; i++) {
        s32 cellX = x + (i % 8) * cellW;
        s32 cellY = y + 15 + (i / 8) * rowH;
        const char* icon = HidHelper::getIconByMask(EDIT_BTN_MASKS[i]);
        bool isSelected = (m_editingButtons & EDIT_BTN_MASKS[i]) != 0;
        tsl::Color iconColor = isSelected ? staticHighlight : tsl::defaultTextColor;
        auto [iconW, iconH] = r->drawString(icon, false, 0, 0, 24, tsl::Color(0,0,0,0));
        r->drawString(icon, false, cellX + (cellW - iconW) / 2, cellY + 28, 24, r->a(iconColor));
        if (i == m_buttonEditIndex) {
            r->drawBorderedRoundedRect(cellX + 3, cellY + 3, cellW - 3, rowH - 10, 3, 2, r->a(highlightColor));
        }
    }
}

bool MacroEditGui::handleButtonEditInput(u64 keysDown) {
    if (keysDown & HidNpadButton_AnyLeft) {
        m_buttonEditIndex = (m_buttonEditIndex % 8 == 0) ? m_buttonEditIndex + 7 : m_buttonEditIndex - 1;
        return true;
    }
    if (keysDown & HidNpadButton_AnyRight) {
        m_buttonEditIndex = (m_buttonEditIndex % 8 == 7) ? m_buttonEditIndex - 7 : m_buttonEditIndex + 1;
        return true;
    }
    if (keysDown & HidNpadButton_AnyUp) {
        m_buttonEditIndex = (m_buttonEditIndex + 8) % 16;
        return true;
    }
    if (keysDown & HidNpadButton_AnyDown) {
        m_buttonEditIndex = (m_buttonEditIndex + 8) % 16;
        return true;
    }
    if (keysDown & HidNpadButton_A) {
        m_editingButtons ^= EDIT_BTN_MASKS[m_buttonEditIndex];
        return true;
    }
    if (keysDown & HidNpadButton_Plus) {
        MacroData::setActionButtons(m_menuSelStart, m_editingButtons);
        m_buttonEditMode = false;
        m_selectMode = false;
        m_menuMode = false;
        return true;
    }
    if (keysDown & HidNpadButton_B) {
        m_buttonEditMode = false;
        return true;
    }
    return true;
}

bool MacroEditGui::handleDurationEditInput(u64 keysDown) {
    auto& actions = MacroData::getActions();
    if (keysDown & HidNpadButton_AnyLeft) {
        m_editDigitIndex = (m_editDigitIndex + 4) % 5;
        return true;
    }
    if (keysDown & HidNpadButton_AnyRight) {
        m_editDigitIndex = (m_editDigitIndex + 1) % 5;
        return true;
    }
    if (keysDown & HidNpadButton_AnyUp) {
        s32 divisor = 1;
        for (int i = 0; i < 4 - m_editDigitIndex; i++) divisor *= 10;
        s32 digit = (m_editingDuration / divisor) % 10;
        s32 maxDigit = (m_editDigitIndex == 0) ? 2 : 9;
        digit = (digit + 1) % (maxDigit + 1);
        m_editingDuration = m_editingDuration - ((m_editingDuration / divisor) % 10) * divisor + digit * divisor;
        return true;
    }
    if (keysDown & HidNpadButton_AnyDown) {
        s32 divisor = 1;
        for (int i = 0; i < 4 - m_editDigitIndex; i++) divisor *= 10;
        s32 digit = (m_editingDuration / divisor) % 10;
        s32 maxDigit = (m_editDigitIndex == 0) ? 2 : 9;
        digit = (digit + maxDigit) % (maxDigit + 1);
        m_editingDuration = m_editingDuration - ((m_editingDuration / divisor) % 10) * divisor + digit * divisor;
        return true;
    }
    if (keysDown & HidNpadButton_A) {
        auto& header = MacroData::getHeader();
        s32 currentActionMs = actions[m_menuSelStart].frameCount * 1000 / header.frameRate;
        s32 totalMs = header.frameCount * 1000 / header.frameRate;
        s32 maxEditMs = 30000 - (totalMs - currentActionMs);
        if (m_editingDuration > maxEditMs) return true;
        MacroData::setActionDuration(m_menuSelStart, m_editingDuration);
        m_durationEditMode = false;
        m_selectMode = false;
        m_menuMode = false;
        return true;
    }
    if (keysDown & HidNpadButton_B) {
        m_durationEditMode = false;
        return true;
    }
    return true;
}

void MacroEditGui::executeDeleteAction() {
    MacroData::deleteActions(m_menuSelStart, m_menuSelEnd);
    m_selectedIndex = (m_menuSelStart > 0) ? m_menuSelStart - 1 : 0;
    s32 itemTop = m_selectedIndex * ITEM_HEIGHT;
    s32 itemBottom = itemTop + ITEM_HEIGHT;
    if (itemTop < m_scrollOffset) m_scrollOffset = itemTop;
    else if (itemBottom > m_scrollOffset + m_listHeight) m_scrollOffset = itemBottom - m_listHeight;
    if (m_scrollOffset < 0) m_scrollOffset = 0;
}

void MacroEditGui::enterDurationEditMode() {
    auto& actions = MacroData::getActions();
    auto& header = MacroData::getHeader();
    m_editingDuration = actions[m_menuSelStart].frameCount * 1000 / header.frameRate;
    m_editDigitIndex = 4;
    m_durationEditMode = true;
}

void MacroEditGui::enterButtonEditMode() {
    auto& actions = MacroData::getActions();
    m_editingButtons = actions[m_menuSelStart].buttons;
    m_buttonEditIndex = 0;
    m_buttonEditMode = true;
}

bool MacroEditGui::handleMenuInput(u64 keysDown) {
    if (keysDown & HidNpadButton_AnyLeft) {
        int next = (m_menuIndex + 5) % 6;
        for (int i = 0; i < 6; i++) {
            if (isMenuItemEnabled(next)) break;
            next = (next + 5) % 6;
        }
        m_menuIndex = next;
        return true;
    }
    if (keysDown & HidNpadButton_AnyRight) {
        int next = (m_menuIndex + 1) % 6;
        for (int i = 0; i < 6; i++) {
            if (isMenuItemEnabled(next)) break;
            next = (next + 1) % 6;
        }
        m_menuIndex = next;
        return true;
    }
    if (keysDown & HidNpadButton_A) {
        if (isMenuItemEnabled(m_menuIndex)) {
            switch (m_menuIndex) {
                case InsertBefore:  MacroData::insertAction(m_menuSelStart, true); break;
                case InsertAfter:   MacroData::insertAction(m_menuSelEnd, false); break;
                case Reset:         MacroData::resetActions(m_menuSelStart, m_menuSelEnd); break;
                case Delete:        executeDeleteAction(); break;
                case EditDuration:  enterDurationEditMode(); return true;
                case EditButton:    enterButtonEditMode(); return true;
            }
            m_selectMode = false;
            m_menuMode = false;
        }
        return true;
    }
    if (keysDown & HidNpadButton_B) {
        m_selectMode = false;
        m_menuMode = false;
        return true;
    }
    return true;
}

bool MacroEditGui::handleNormalInput(u64 keysDown, u64 keysHeld) {
    auto& actions = MacroData::getActions();
    s32 maxIndex = actions.size() - 1;
    
    if (keysDown & HidNpadButton_A) {
        if (!m_selectMode) {
            m_selectMode = true;
            m_selectAnchor = m_selectedIndex;
        }
        return true;
    }
    if (keysDown & HidNpadButton_B) {
        if (m_selectMode) {
            m_selectMode = false;
            m_selectAnchor = -1;
            return true;
        }
    }
    if (keysDown & HidNpadButton_X) {
        if (MacroData::undo()) {
            if (m_selectedIndex >= (s32)actions.size()) m_selectedIndex = actions.size() - 1;
            m_selectMode = false;
            m_selectAnchor = -1;
            return true;
        }
    }
    if (keysDown & HidNpadButton_Plus) {
        if (!m_selectMode) {
            MacroData::saveForEdit(m_macroFilePath);
            Refresh::RefrRequest(Refresh::MacroGameList); 
            g_ipcManager.sendReloadMacroCommand();
            tsl::swapTo<MacroViewGui>(SwapDepth(2), m_macroFilePath, m_gameName, m_isRecord);
            return true;
        }
    }
    if (keysDown & HidNpadButton_Minus) {
        if (m_selectMode) {
            m_menuSelStart = std::min(m_selectAnchor, m_selectedIndex);
            m_menuSelEnd = std::max(m_selectAnchor, m_selectedIndex);
            m_menuMode = true;
            m_menuIndex = 0;
            while (!isMenuItemEnabled(m_menuIndex) && m_menuIndex < 5) m_menuIndex++;
            return true;
        }
    }
    
    bool moveUp = false, moveDown = false;
    if (keysDown & HidNpadButton_AnyUp) { moveUp = true; m_repeatCounter = 0; }
    else if (keysHeld & HidNpadButton_AnyUp) {
        m_repeatCounter++;
        if (m_repeatCounter > REPEAT_DELAY && (m_repeatCounter - REPEAT_DELAY) % REPEAT_RATE == 0) moveUp = true;
    }
    if (keysDown & HidNpadButton_AnyDown) { moveDown = true; m_repeatCounter = 0; }
    else if (keysHeld & HidNpadButton_AnyDown) {
        m_repeatCounter++;
        if (m_repeatCounter > REPEAT_DELAY && (m_repeatCounter - REPEAT_DELAY) % REPEAT_RATE == 0) moveDown = true;
    }
    if (!(keysHeld & (HidNpadButton_AnyUp | HidNpadButton_AnyDown))) m_repeatCounter = 0;
    
    if (moveUp && m_selectedIndex > 0) {
        m_selectedIndex--;
        s32 itemTop = m_selectedIndex * ITEM_HEIGHT;
        if (itemTop < m_scrollOffset + ITEM_HEIGHT) {
            m_scrollOffset -= ITEM_HEIGHT;
            if (m_scrollOffset < 0) m_scrollOffset = 0;
        }
        return true;
    }
    if (moveDown && m_selectedIndex < maxIndex) {
        m_selectedIndex++;
        s32 itemBottom = (m_selectedIndex + 1) * ITEM_HEIGHT;
        if (itemBottom > m_scrollOffset + m_listHeight) m_scrollOffset += ITEM_HEIGHT;
        return true;
    }
    return false;
}

bool MacroEditGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (m_buttonEditMode) return handleButtonEditInput(keysDown);
    if (m_durationEditMode) return handleDurationEditInput(keysDown);
    if (m_menuMode) return handleMenuInput(keysDown);
    return handleNormalInput(keysDown, keysHeld);
}
