#include "macro_edit.hpp"
#include "macro_data.hpp"
#include "hiddata.hpp"
#include <ultra.hpp>
#include "refresh.hpp"
#include "ipc.hpp"
#include "macro_view.hpp"

namespace {
    // 摇杆方向图标
    const char* StickDirIcons[] = {"", "", "", "", "", "", "", "", ""};
    
    // 获取摇杆方向图标
    const char* getStickDirIcon(StickDir dir) {
        return StickDirIcons[static_cast<int>(dir)];
    }
    
    // 生成动作显示文本
    std::string getActionText(const Action& action) {
        std::string text;
        
        // 左摇杆
        if (action.stickL != StickDir::None) {
            text += "";
            text += getStickDirIcon(action.stickL);
        }
        
        // 右摇杆
        if (action.stickR != StickDir::None) {
            if (!text.empty()) text += "+";
            text += "";
            text += getStickDirIcon(action.stickR);
        }
        
        // 按钮
        if (action.buttons != 0) {
            if (!text.empty()) text += "+";
            text += HidHelper::getCombinedIcons(action.buttons);
        }
        
        // 无动作
        if (text.empty()) text = "无动作";
        
        return text;
    }
}

MacroEditGui::MacroEditGui(const char* macroFilePath, const char* gameName) 
 : m_macroFilePath(macroFilePath)
 , m_gameName(gameName)
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
        
        // 底部按钮（使用绝对坐标）
        const char* btnText = m_selectMode ? "  修改" : "  保存";
        r->drawString(btnText, false, 280, 693, 23, r->a(tsl::style::color::ColorText));
    }));
    
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        auto& actions = MacroData::getActions();
        auto& header = MacroData::getHeader();
        
        // 计算闪烁进度
        u64 currentTime_ns = armTicksToNs(armGetSystemTick());
        double progress = (std::cos(2.0 * 3.14159265358979323846 * std::fmod(currentTime_ns * 0.000000001 - 0.25, 1.0)) + 1.0) * 0.5;
        
        // 闪烁高亮色（青色）
        tsl::Color highlightColor = tsl::Color(
            static_cast<u8>(tsl::highlightColor2.r + (tsl::highlightColor1.r - tsl::highlightColor2.r) * progress),
            static_cast<u8>(tsl::highlightColor2.g + (tsl::highlightColor1.g - tsl::highlightColor2.g) * progress),
            static_cast<u8>(tsl::highlightColor2.b + (tsl::highlightColor1.b - tsl::highlightColor2.b) * progress),
            0xF);
        
        // 1. 绘制时间轴区域（或菜单/编辑模式）
        if (m_buttonEditMode) {
            // ===== 按键编辑模式 =====
            const u64 editBtnMasks[16] = {
                BTN_A, BTN_B, BTN_X, BTN_Y, BTN_L, BTN_R, BTN_ZL, BTN_ZR,
                BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_SELECT, BTN_START, BTN_STICKL, BTN_STICKR
            };
            
            s32 cellW = w / 8;
            s32 rowH = 40;
            
            // 静态高亮色（选中按钮用，不闪烁）
            tsl::Color staticHighlight = tsl::Color(0x0, 0xF, 0xF, 0xF);
            
            for (int i = 0; i < 16; i++) {
                s32 row = i / 8;
                s32 col = i % 8;
                s32 cellX = x + col * cellW;
                s32 cellY = y + 15 + row * rowH;
                s32 centerX = cellX + cellW / 2;
                s32 centerY = cellY + rowH / 2;
                
                const char* icon = HidHelper::getIconByMask(editBtnMasks[i]);
                bool isSelected = (m_editingButtons & editBtnMasks[i]) != 0;
                bool isCursor = (i == m_buttonEditIndex);
                
                // 图标颜色：选中为静态高亮色，未选中为白色
                tsl::Color iconColor = isSelected ? staticHighlight : tsl::defaultTextColor;
                
                // 绘制图标（居中）
                auto [iconW, iconH] = r->drawString(icon, false, 0, 0, 24, tsl::Color(0,0,0,0));
                r->drawString(icon, false, cellX + (cellW - iconW) / 2, cellY + 28, 24, r->a(iconColor));
                
                // 光标位置绘制闪烁矩形边框
                if (isCursor) {
                    r->drawBorderedRoundedRect(cellX + 3, cellY + 3, cellW - 3, rowH - 10, 3, 2, r->a(highlightColor));
                }
            }
            
        } else if (m_durationEditMode) {
            // ===== 时长编辑模式 =====
            // 计算最大可编辑时长（30秒上限）
            s32 currentActionMs = actions[m_menuSelStart].frameCount * 1000 / header.frameRate;
            s32 totalMs = header.frameCount * 1000 / header.frameRate;
            s32 maxEditMs = 30000 - (totalMs - currentActionMs);
            bool isOverLimit = m_editingDuration > maxEditMs;
            
            s32 digits[5] = {
                m_editingDuration / 10000 % 10,
                m_editingDuration / 1000 % 10,
                m_editingDuration / 100 % 10,
                m_editingDuration / 10 % 10,
                m_editingDuration % 10
            };
            
            // 绘制5个数字和"ms"（间隙统一）
            s32 digitW = 30;
            s32 gap = 8;  // 数字之间的间隙
            s32 totalW = digitW * 5 + gap * 5 + 30;  // 5个数字 + 5个间隙 + "ms"宽度
            s32 startX = x + (w - totalW) / 2;
            
            // 超过上限时全部显示红色，选中位闪烁红色
            tsl::Color errorColor = tsl::Color(0xF, 0x2, 0x2, 0xF);
            tsl::Color errorHighlight = tsl::Color(
                static_cast<u8>(0x8 + (0xF - 0x8) * progress),
                static_cast<u8>(0x1 + (0x4 - 0x1) * progress),
                static_cast<u8>(0x1 + (0x4 - 0x1) * progress),
                0xF);
            for (int i = 0; i < 5; i++) {
                char buf[2] = {(char)('0' + digits[i]), '\0'};
                tsl::Color color = isOverLimit ? 
                                   ((i == m_editDigitIndex) ? errorHighlight : errorColor) : 
                                   ((i == m_editDigitIndex) ? highlightColor : tsl::defaultTextColor);
                r->drawString(buf, false, startX + i * (digitW + gap), y + 60, 36, r->a(color));
            }
            r->drawString("ms", false, startX + 5 * (digitW + gap), y + 60, 28, 
                          r->a(isOverLimit ? errorColor : tsl::defaultTextColor));
            
            // 溢出时在右下角显示最大允许时间（右对齐，与列表ms对齐）
            if (isOverLimit) {
                char maxBuf[32];
                snprintf(maxBuf, sizeof(maxBuf), "最大: %dms", maxEditMs);
                auto [maxW, maxH] = r->drawString(maxBuf, false, 0, 0, 14, tsl::Color(0,0,0,0));
                r->drawString(maxBuf, false, x + w - 15 - maxW, y + 90, 14, r->a(errorColor));
            }
            
        } else if (m_menuMode) {
            // ===== 菜单模式：绘制横向菜单 =====
            
            s32 itemW = w / 6;
            const char* icons[] = {"", "", "", "", "", ""};
            
            for (int i = 0; i < 6; i++) {
                s32 itemX = x + i * itemW;
                bool enabled = isMenuItemEnabled(i);
                // 图标（水平垂直居中，不可选项显示灰色）
                tsl::Color iconColor = enabled ? tsl::defaultTextColor : tsl::Color(0x5, 0x5, 0x5, 0xF);
                auto [iconW, iconH] = r->drawString(icons[i], false, 0, 0, 30, tsl::Color(0,0,0,0));
                r->drawString(icons[i], false, itemX + (itemW - iconW) / 2, y + 58, 30, r->a(iconColor));
                
                // 分界线（除了最后一个）
                if (i < 5) {
                    r->drawRect(itemX + itemW, y + 22, 1, 56, r->a(tsl::separatorColor));
                }

                // 选中项绘制闪烁圆角边框（与图标垂直居中对齐）
                if (i == m_menuIndex) {
                    r->drawBorderedRoundedRect(itemX, y + 7, itemW + 5, 73, 5, 5, r->a(highlightColor));
                }
            }
        } else {
            // ===== 正常模式：绘制时间轴 =====
            float totalDuration = (float)header.frameCount / header.frameRate;
            float pixelPerSec = (float)(w - 20) / totalDuration;
            
            s32 barH = 25;
            s32 barY = y + 50;
            s32 tickY = y + 27;
            
            // 绘制时间刻度
            s32 tickInterval = (totalDuration > 20) ? 5 : (totalDuration > 10) ? 2 : 1;
            for (s32 t = 0; t <= (s32)totalDuration; t += tickInterval) {
                s32 tickX = x + 10 + (s32)(t * pixelPerSec);
                r->drawRect(tickX, tickY + 5, 1, 10, r->a(tsl::Color(0x8, 0x8, 0x8, 0xF)));
                char buf[8];
                snprintf(buf, sizeof(buf), "%ds", t);
                r->drawString(buf, false, tickX - 5, tickY, 12, r->a(tsl::Color(0xA, 0xA, 0xA, 0xF)));
            }
            
            // 选择模式高亮色（黄色闪烁）
            tsl::Color timelineHighlight = m_selectMode
                ? tsl::Color(
                    static_cast<u8>(0x8 + (0xF - 0x8) * progress),
                    static_cast<u8>(0x6 + (0xD - 0x6) * progress),
                    0x0,
                    0xF)
                : highlightColor;
            
            // 计算选择范围
            s32 selStart = m_selectedIndex, selEnd = m_selectedIndex;
            if (m_selectMode && m_selectAnchor >= 0) {
                selStart = std::min(m_selectAnchor, m_selectedIndex);
                selEnd = std::max(m_selectAnchor, m_selectedIndex);
            }
            
            // 绘制动作色块
            float currentTime = 0;
            s32 rangeStartX = -1, rangeEndX = -1;
            for (size_t i = 0; i < actions.size(); i++) {
                float duration = (float)actions[i].frameCount / header.frameRate;
                s32 blockX = x + 10 + (s32)(currentTime * pixelPerSec);
                s32 blockW = (s32)(duration * pixelPerSec);
                if (blockW < 1) blockW = 1;
                
                bool inRange = ((s32)i >= selStart && (s32)i <= selEnd);
                s32 blockDrawY = inRange ? barY - 3 : barY;
                s32 blockDrawH = inRange ? barH + 6 : barH;
                
                tsl::Color blockColor = (actions[i].buttons != 0) 
                    ? tsl::Color(0x2, 0xA, 0x2, 0xF)
                    : (actions[i].stickL != StickDir::None || actions[i].stickR != StickDir::None)
                        ? tsl::Color(0xA, 0x2, 0x2, 0xF)
                        : tsl::Color(0x8, 0x8, 0x8, 0xF);
                
                r->drawRect(blockX, blockDrawY, blockW, blockDrawH, r->a(blockColor));
                
                if ((s32)i == selStart) rangeStartX = blockX;
                if ((s32)i == selEnd) rangeEndX = blockX + blockW;
                
                currentTime += duration;
            }
            
            // 绘制选中框
            if (rangeStartX >= 0 && rangeEndX >= 0) {
                s32 boxX = rangeStartX - 2;
                s32 boxW = rangeEndX - rangeStartX + 4;
                s32 boxY = barY - 3;
                s32 boxH = barH + 6;
                r->drawRect(boxX, boxY, boxW, 2, r->a(timelineHighlight));
                r->drawRect(boxX, boxY + boxH, boxW, 2, r->a(timelineHighlight));
                r->drawRect(boxX, boxY, 2, boxH, r->a(timelineHighlight));
                r->drawRect(rangeEndX, boxY, 2, boxH, r->a(timelineHighlight));
            }
        }
        
        // 底部分隔线
        r->drawRect(x, y + 100 - 1, w, 1, r->a(tsl::separatorColor));
        
        // 2. 绘制动作列表（底部留出 73px 给按钮区域）
        s32 listY = y + 100 + 5;  // 起点增加5
        s32 listH = h - 100 - 73;
        m_listHeight = listH;  // 保存可见区域高度供 handleInput 使用
        
        for (size_t i = 0; i < actions.size(); i++) {
            s32 itemY = listY + i * ITEM_HEIGHT - m_scrollOffset;
            
            // 裁剪：只绘制可见区域（底部留出 73px 给按钮区域）
            s32 bottomY = y + h - 73;
            if (itemY + ITEM_HEIGHT > listY && itemY < bottomY) {
                // 分隔线（Tesla 风格）
                r->drawRect(x + 4, itemY + ITEM_HEIGHT - 1, w - 8, 1, r->a(tsl::separatorColor));
                
                // 选中框（Tesla 风格 + 闪烁效果）
                bool isSelected = ((s32)i == m_selectedIndex);
                if (isSelected) {
                    // 普通模式：青色闪烁；选择模式：黄色闪烁
                    tsl::Color highlightColor = tsl::Color(0, 0, 0, 0);
                    if (m_selectMode) {
                        highlightColor = tsl::Color(
                            static_cast<u8>(0x8 + (0xF - 0x8) * progress),
                            static_cast<u8>(0x6 + (0xD - 0x6) * progress),
                            0x0,
                            0xF);
                    } else {
                        highlightColor = tsl::Color(
                            static_cast<u8>(tsl::highlightColor2.r + (tsl::highlightColor1.r - tsl::highlightColor2.r) * progress),
                            static_cast<u8>(tsl::highlightColor2.g + (tsl::highlightColor1.g - tsl::highlightColor2.g) * progress),
                            static_cast<u8>(tsl::highlightColor2.b + (tsl::highlightColor1.b - tsl::highlightColor2.b) * progress),
                            0xF);
                    }
                    
                    r->drawBorderedRoundedRect(x, itemY, w + 4, ITEM_HEIGHT, 5, 5, r->a(highlightColor));
                }
                
                // 动作文本（Tesla 风格位置：x+19, y+45）
                std::string text = getActionText(actions[i]);
                u32 durationMs = actions[i].frameCount * 1000 / header.frameRate;
                std::string duration = std::to_string(durationMs) + "ms";
                
                // 序号 + 主文字：选中范围内为黄色，修改过为粉色，否则白色
                std::string fullText = std::to_string(i + 1) + ". " + text;
                bool inSelectRange = false;
                if (m_selectMode && m_selectAnchor >= 0) {
                    s32 selStart = std::min(m_selectAnchor, m_selectedIndex);
                    s32 selEnd = std::max(m_selectAnchor, m_selectedIndex);
                    inSelectRange = ((s32)i >= selStart && (s32)i <= selEnd);
                }
                tsl::Color textColor = inSelectRange ? tsl::Color(0xF, 0xD, 0x0, 0xF) 
                                     : (actions[i].modified ? tsl::Color(0xF, 0xA, 0xC, 0xF) : tsl::defaultTextColor);
                // 时长宽度（用于计算主文字最大宽度）
                auto [durW, durH] = r->drawString(duration.c_str(), false, 0, 0, 20, tsl::Color(0,0,0,0));
                // 截断主文字避免与时长重叠
                s32 maxTextWidth = w - 19 - durW - 25;
                std::string truncatedText = r->limitStringLength(fullText, false, 23, maxTextWidth);
                r->drawString(truncatedText.c_str(), false, x + 19, itemY + 45, 23, r->a(textColor));
                // 时长右对齐
                r->drawString(duration.c_str(), false, x + w - 15 - durW, itemY + 45, 20, r->a(tsl::onTextColor));
            }
        }
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

bool MacroEditGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    auto& actions = MacroData::getActions();
    s32 maxIndex = actions.size() - 1;
    
    // ===== 按键编辑模式输入处理 =====
    if (m_buttonEditMode) {
        const u64 editBtnMasks[16] = {
            BTN_A, BTN_B, BTN_X, BTN_Y, BTN_L, BTN_R, BTN_ZL, BTN_ZR,
            BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_SELECT, BTN_START, BTN_STICKL, BTN_STICKR
        };
        
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
            // 切换当前按键选中状态
            m_editingButtons ^= editBtnMasks[m_buttonEditIndex];
            return true;
        }
        if (keysDown & HidNpadButton_Plus) {
            // 保存
            MacroData::setActionButtons(m_menuSelStart, m_editingButtons);
            m_buttonEditMode = false;
            m_selectMode = false;
            m_menuMode = false;
            return true;
        }
        if (keysDown & HidNpadButton_B) {
            m_buttonEditMode = false;  // 取消返回菜单
            return true;
        }
        return true;
    }
    
    // ===== 时长编辑模式输入处理 =====
    if (m_durationEditMode) {
        if (keysDown & HidNpadButton_AnyLeft) {
            m_editDigitIndex = (m_editDigitIndex + 4) % 5;
            return true;
        }
        if (keysDown & HidNpadButton_AnyRight) {
            m_editDigitIndex = (m_editDigitIndex + 1) % 5;
            return true;
        }
        if (keysDown & HidNpadButton_AnyUp) {
            // 当前位循环（第一位0-2，其他位0-9）
            s32 divisor = 1;
            for (int i = 0; i < 4 - m_editDigitIndex; i++) divisor *= 10;
            s32 digit = (m_editingDuration / divisor) % 10;
            s32 maxDigit = (m_editDigitIndex == 0) ? 2 : 9;
            digit = (digit + 1) % (maxDigit + 1);
            m_editingDuration = m_editingDuration - ((m_editingDuration / divisor) % 10) * divisor + digit * divisor;
            return true;
        }
        if (keysDown & HidNpadButton_AnyDown) {
            // 当前位循环（第一位0-2，其他位0-9）
            s32 divisor = 1;
            for (int i = 0; i < 4 - m_editDigitIndex; i++) divisor *= 10;
            s32 digit = (m_editingDuration / divisor) % 10;
            s32 maxDigit = (m_editDigitIndex == 0) ? 2 : 9;
            digit = (digit + maxDigit) % (maxDigit + 1);  // +maxDigit 相当于 -1 mod (maxDigit+1)
            m_editingDuration = m_editingDuration - ((m_editingDuration / divisor) % 10) * divisor + digit * divisor;
            return true;
        }
        if (keysDown & HidNpadButton_A) {
            // 检查30秒上限
            auto& header = MacroData::getHeader();
            s32 currentActionMs = actions[m_menuSelStart].frameCount * 1000 / header.frameRate;
            s32 totalMs = header.frameCount * 1000 / header.frameRate;
            s32 maxEditMs = 30000 - (totalMs - currentActionMs);
            if (m_editingDuration > maxEditMs) {
                return true;  // 超过上限，A键无效
            }
            MacroData::setActionDuration(m_menuSelStart, m_editingDuration);
            m_durationEditMode = false;
            m_selectMode = false;
            m_menuMode = false;
            return true;
        }
        if (keysDown & HidNpadButton_B) {
            m_durationEditMode = false;  // 回到菜单
            return true;
        }
        return true;
    }
    
    // ===== 菜单模式输入处理 =====
    if (m_menuMode) {
        if (keysDown & HidNpadButton_AnyLeft) {
            // 向左找下一个可选项
            int next = (m_menuIndex + 5) % 6;
            for (int i = 0; i < 6; i++) {
                if (isMenuItemEnabled(next)) break;
                next = (next + 5) % 6;
            }
            m_menuIndex = next;
            return true;
        }
        if (keysDown & HidNpadButton_AnyRight) {
            // 向右找下一个可选项
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
                    case 0:  // 在前面插入新动作
                        MacroData::insertAction(m_menuSelStart, true);
                        break;
                    case 1:  // 在后面插入新动作
                        MacroData::insertAction(m_menuSelEnd, false);
                        break;
                    case 2:  // 重置动作
                        MacroData::resetActions(m_menuSelStart, m_menuSelEnd);
                        break;
                    case 3:  // 删除动作
                        MacroData::deleteActions(m_menuSelStart, m_menuSelEnd);
                        // 更新光标位置：优先向上一格，否则第一格
                        if (m_menuSelStart > 0) {
                            m_selectedIndex = m_menuSelStart - 1;
                        } else {
                            m_selectedIndex = 0;
                        }
                        // 更新滚动位置，只在必要时调整
                        {
                            s32 itemTop = m_selectedIndex * ITEM_HEIGHT;
                            s32 itemBottom = itemTop + ITEM_HEIGHT;
                            if (itemTop < m_scrollOffset) {
                                m_scrollOffset = itemTop;
                            } else if (itemBottom > m_scrollOffset + m_listHeight) {
                                m_scrollOffset = itemBottom - m_listHeight;
                            }
                            if (m_scrollOffset < 0) m_scrollOffset = 0;
                        }
                        break;
                    case 4:  // 修改持续时间
                        {
                            auto& header = MacroData::getHeader();
                            m_editingDuration = actions[m_menuSelStart].frameCount * 1000 / header.frameRate;
                            m_editDigitIndex = 4;  // 默认选中个位（5位数字，索引4）
                            m_durationEditMode = true;
                        }
                        return true;  // 不退出菜单模式，进入时长编辑
                    case 5:  // 修改触发按钮
                        m_editingButtons = actions[m_menuSelStart].buttons;
                        m_buttonEditIndex = 0;
                        m_buttonEditMode = true;
                        return true;  // 不退出菜单模式，进入按键编辑
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
        return true;  // 菜单模式下阻止其他输入
    }
    
    // ===== 正常模式输入处理 =====
    
    // A键：进入选择模式，记录起点
    if (keysDown & HidNpadButton_A) {
        if (!m_selectMode) {
            m_selectMode = true;
            m_selectAnchor = m_selectedIndex;
        }
        return true;
    }
    
    // B键：退出选择模式
    if (keysDown & HidNpadButton_B) {
        if (m_selectMode) {
            m_selectMode = false;
            m_selectAnchor = -1;
            return true;
        }
    }
    
    // X键：撤销上一次操作
    if (keysDown & HidNpadButton_X) {
        if (MacroData::undo()) {
            // 确保光标在有效范围内
            if (m_selectedIndex >= (s32)actions.size()) {
                m_selectedIndex = actions.size() - 1;
            }
            m_selectMode = false;
            m_selectAnchor = -1;
            return true;
        }
    }
    
    // +键：非选择模式下保存
    if (keysDown & HidNpadButton_Plus) {
        if (!m_selectMode) {
            MacroData::save(m_macroFilePath);
            Refresh::RefrRequest(Refresh::MacroGameList); 
            g_ipcManager.sendReloadMacroCommand();
            tsl::swapTo<MacroViewGui>(SwapDepth(2), m_macroFilePath, m_gameName);
            return true;
        }
    }

    // -键：选择模式下打开菜单
    if (keysDown & HidNpadButton_Minus) {
        if (m_selectMode) {
            m_menuSelStart = std::min(m_selectAnchor, m_selectedIndex);
            m_menuSelEnd = std::max(m_selectAnchor, m_selectedIndex);
            m_menuMode = true;
            m_menuIndex = 0;
            // 如果初始位置不可选，找下一个可选的
            while (!isMenuItemEnabled(m_menuIndex) && m_menuIndex < 5) {
                m_menuIndex++;
            }
            return true;
        }
    }
    
    // 检查是否需要移动（首次按下或重复触发）
    bool moveUp = false, moveDown = false;
    
    if (keysDown & HidNpadButton_AnyUp) {
        moveUp = true;
        m_repeatCounter = 0;
    } else if (keysHeld & HidNpadButton_AnyUp) {
        m_repeatCounter++;
        if (m_repeatCounter > REPEAT_DELAY && (m_repeatCounter - REPEAT_DELAY) % REPEAT_RATE == 0) {
            moveUp = true;
        }
    }
    
    if (keysDown & HidNpadButton_AnyDown) {
        moveDown = true;
        m_repeatCounter = 0;
    } else if (keysHeld & HidNpadButton_AnyDown) {
        m_repeatCounter++;
        if (m_repeatCounter > REPEAT_DELAY && (m_repeatCounter - REPEAT_DELAY) % REPEAT_RATE == 0) {
            moveDown = true;
        }
    }
    
    // 松开按键时重置计数器
    if (!(keysHeld & (HidNpadButton_AnyUp | HidNpadButton_AnyDown))) {
        m_repeatCounter = 0;
    }
    
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
        if (itemBottom > m_scrollOffset + m_listHeight) {
            m_scrollOffset += ITEM_HEIGHT;
        }
        return true;
    }
    
    return false;
}
