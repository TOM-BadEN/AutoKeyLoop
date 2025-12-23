#include "macro_detail.hpp"
#include <ultra.hpp>
#include <cmath>
#include "language.hpp"
#include "info_edit.hpp"

namespace {
    constexpr s32 ITEM_HEIGHT = 70;
    
    // 使用说明布局常量（参考 updater_ui.cpp）
    constexpr s32 DESC_FONT_SIZE = 18;
    constexpr s32 DESC_LINE_HEIGHT = 26;
    constexpr s32 SCROLLBAR_WIDTH = 3;
    constexpr s32 SCROLLBAR_GAP = 8;
    
    constexpr const char* EDIT_URL = "https://macro.dokiss.cn/edit_prop.php?tid=%s&file=%s&lang=%s";
    constexpr const char* langParam[] = {"zh", "zhh", "en"};
    
    // 使用说明行数据
    struct DescLine {
        std::string text;
        bool hasPrefix;
    };
    
    // 获取 UTF-8 字符边界
    std::vector<size_t> getUtf8CharBounds(const std::string& text) {
        std::vector<size_t> bounds = {0};
        for (size_t i = 0; i < text.size(); ) {
            unsigned char c = text[i];
            if ((c & 0x80) == 0) i += 1;
            else if ((c & 0xE0) == 0xC0) i += 2;
            else if ((c & 0xF0) == 0xE0) i += 3;
            else i += 4;
            bounds.push_back(i);
        }
        return bounds;
    }
    
    // 二分查找文本截断点
    size_t findTextCutPoint(tsl::gfx::Renderer* r, const std::string& text,
                            const std::vector<size_t>& bounds, s32 maxWidth, s32 fontSize) {
        if (bounds.size() <= 1) return text.size();
        size_t lo = 1, hi = bounds.size() - 1, cutIdx = 1;
        while (lo <= hi) {
            size_t mid = (lo + hi) / 2;
            auto [mw, mh] = r->getTextDimensions(text.substr(0, bounds[mid]), false, fontSize);
            if (mw <= maxWidth) { cutIdx = mid; lo = mid + 1; }
            else { hi = mid - 1; }
        }
        return bounds[cutIdx];
    }
    
    // 预处理使用说明为行数据
    std::vector<DescLine> preprocessDesc(
        tsl::gfx::Renderer* r, const std::string& desc,
        s32 maxWidth, s32 prefixW, s32& outTotalHeight) {
        
        std::vector<DescLine> lines;
        outTotalHeight = 0;
        
        // 按 \\n 分割
        std::string remaining = desc;
        size_t pos = 0;
        while ((pos = remaining.find("\\n")) != std::string::npos) {
            std::string item = remaining.substr(0, pos);
            remaining = remaining.substr(pos + 2);
            
            // 换行处理
            bool isFirstLine = true;
            while (!item.empty()) {
                s32 lineMaxWidth = isFirstLine ? maxWidth : (maxWidth - prefixW);
                auto [tw, th] = r->getTextDimensions(item, false, DESC_FONT_SIZE);
                
                if (tw <= lineMaxWidth) {
                    lines.push_back({item, isFirstLine});
                    outTotalHeight += DESC_LINE_HEIGHT;
                    break;
                }
                
                auto bounds = getUtf8CharBounds(item);
                size_t cut = findTextCutPoint(r, item, bounds, lineMaxWidth, DESC_FONT_SIZE);
                lines.push_back({item.substr(0, cut), isFirstLine});
                outTotalHeight += DESC_LINE_HEIGHT;
                item = item.substr(cut);
                isFirstLine = false;
            }
        }
        
        // 处理最后一段
        if (!remaining.empty()) {
            bool isFirstLine = true;
            while (!remaining.empty()) {
                s32 lineMaxWidth = isFirstLine ? maxWidth : (maxWidth - prefixW);
                auto [tw, th] = r->getTextDimensions(remaining, false, DESC_FONT_SIZE);
                
                if (tw <= lineMaxWidth) {
                    lines.push_back({remaining, isFirstLine});
                    outTotalHeight += DESC_LINE_HEIGHT;
                    break;
                }
                
                auto bounds = getUtf8CharBounds(remaining);
                size_t cut = findTextCutPoint(r, remaining, bounds, lineMaxWidth, DESC_FONT_SIZE);
                lines.push_back({remaining.substr(0, cut), isFirstLine});
                outTotalHeight += DESC_LINE_HEIGHT;
                remaining = remaining.substr(cut);
                isFirstLine = false;
            }
        }
        
        return lines;
    }
    
    double calcBlinkProgress() {
        u64 ns = armTicksToNs(armGetSystemTick());
        return (std::cos(2.0 * 3.14159265358979323846 * std::fmod(ns * 0.000000001 - 0.25, 1.0)) + 1.0) * 0.5;
    }
    
    tsl::Color calcBlinkColor(tsl::Color c1, tsl::Color c2, double progress) {
        return tsl::Color(
            static_cast<u8>(c2.r + (c1.r - c2.r) * progress),
            static_cast<u8>(c2.g + (c1.g - c2.g) * progress),
            static_cast<u8>(c2.b + (c1.b - c2.b) * progress), 0xF);
    }
}

// 脚本详情界面
MacroDetailGui::MacroDetailGui(const std::string& macroFilePath, const std::string& gameName)
 : m_filePath(macroFilePath), m_gameName(gameName)
{
    m_meta = MacroUtil::getMetadata(macroFilePath.c_str());
}

tsl::elm::Element* MacroDetailGui::createUI() {
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    frame->setHeader(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString(m_gameName, false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString("脚本的相关介绍", false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
    }));
    auto list = new tsl::elm::List();
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        drawDetail(r, x, y, w, h);
    });
    list->addItem(drawer, 520);
    frame->setContent(list);
    return frame;
}

void MacroDetailGui::drawDetail(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 textX = x + 19;
    s32 currentY = y + 35;
    
    // 脚本名称 + 作者（没有name则显示文件名，去掉.macro后缀）
    std::string displayName = m_meta.name;
    if (displayName.empty()) {
        displayName = ult::getFileName(m_filePath);
        if (displayName.size() > 6) displayName = displayName.substr(0, displayName.size() - 6);
    }
    s32 nameFontSize = 28;
    s32 byFontSize = 20;
    std::string byText = m_meta.author.empty() ? "" : " by " + m_meta.author;
    auto nameDim = r->getTextDimensions(displayName, false, nameFontSize);
    auto byDim = r->getTextDimensions(byText, false, byFontSize);
    while (nameDim.first + byDim.first > w - 19 - 15) {
        nameFontSize = nameFontSize * 9 / 10;
        byFontSize = byFontSize * 9 / 10;
        nameDim = r->getTextDimensions(displayName, false, nameFontSize);
        byDim = r->getTextDimensions(byText, false, byFontSize);
    }
    r->drawString(displayName, false, textX, currentY, nameFontSize, r->a(tsl::onTextColor));
    if (!byText.empty()) {
        r->drawString(byText, false, textX + nameDim.first, currentY, byFontSize, r->a(tsl::onTextColor));
    }
    currentY += 50;
    
    // 通用换行绘制函数
    s32 maxWidth = w - 19 - 15;
    s32 fontSize = 18;
    s32 lineHeight = 26;
    s32 listY = y + h - 10 - ITEM_HEIGHT;
    s32 maxY = listY - 20;
    
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
    
    // ========== 使用说明（可滚动区域）==========
    r->drawString("使用说明:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    
    // 计算区域边界（参考 updater_ui.cpp L314-318）
    s32 textMinY = currentY;
    s32 descMaxY = listY - 22;
    s32 visibleHeight = descMaxY - textMinY;
    s32 scrollMinY = textMinY - DESC_LINE_HEIGHT + 5;
    
    // 预处理使用说明
    std::string prefix = " • ";
    auto [prefixW, prefixH] = r->getTextDimensions(prefix, false, DESC_FONT_SIZE);
    s32 descMaxWidth = maxWidth - SCROLLBAR_WIDTH - SCROLLBAR_GAP;
    
    if (m_meta.desc.empty()) {
        r->drawString(" • 暂无使用说明", false, textX, currentY, DESC_FONT_SIZE, r->a(tsl::style::color::ColorDescription));
    } else {
        s32 totalContentHeight = 0;
        auto lines = preprocessDesc(r, m_meta.desc, descMaxWidth, prefixW, totalContentHeight);
        
        // 更新滚动状态（参考 updater_ui.cpp L327-329）
        m_maxScrollOffset = (totalContentHeight > visibleHeight) ? (totalContentHeight - visibleHeight) : 0;
        if (m_scrollOffset > m_maxScrollOffset) m_scrollOffset = m_maxScrollOffset;
        if (m_scrollOffset < 0) m_scrollOffset = 0;
        
        // 绘制使用说明（参考 updater_ui.cpp L332-347）
        s32 maxLines = (visibleHeight + DESC_LINE_HEIGHT - 1) / DESC_LINE_HEIGHT;
        s32 contentY = 0, drawY = textMinY, drawnLines = 0;
        
        for (const auto& line : lines) {
            if (drawnLines >= maxLines) break;
            if (contentY >= m_scrollOffset) {
                if (line.hasPrefix) {
                    r->drawString(prefix + line.text, false, textX, drawY, DESC_FONT_SIZE, r->a(tsl::style::color::ColorDescription));
                } else {
                    r->drawString(line.text, false, textX + prefixW, drawY, DESC_FONT_SIZE, r->a(tsl::style::color::ColorDescription));
                }
                drawY += DESC_LINE_HEIGHT;
                drawnLines++;
            }
            contentY += DESC_LINE_HEIGHT;
        }
        
        // 绘制滚动条（参考 updater_ui.cpp L350-360）
        if (m_maxScrollOffset > 0) {
            s32 scrollBarX = x + w - SCROLLBAR_WIDTH;
            s32 scrollBarTotalH = descMaxY - scrollMinY;
            s32 scrollBarHeight = scrollBarTotalH * visibleHeight / totalContentHeight;
            if (scrollBarHeight < 20) scrollBarHeight = 20;
            s32 scrollBarY = scrollMinY + (scrollBarTotalH - scrollBarHeight) * m_scrollOffset / m_maxScrollOffset;
            s32 radius = SCROLLBAR_WIDTH / 2;
            
            r->drawRoundedRect(scrollBarX, scrollMinY, SCROLLBAR_WIDTH, scrollBarTotalH, radius, tsl::Color(0x3, 0x3, 0x3, 0xD));
            r->drawRoundedRect(scrollBarX, scrollBarY, SCROLLBAR_WIDTH, scrollBarHeight, radius, tsl::Color(0x8, 0x8, 0x8, 0xF));
        }
    }
    
    // ========== 底部按钮 ==========
    double progress = calcBlinkProgress();
    tsl::Color hlColor = calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
    r->drawBorderedRoundedRect(x, listY, w + 4, ITEM_HEIGHT, 5, 5, r->a(hlColor));
    
    s32 keyFont = 23;
    s32 valFont = 20;
    std::string keyText = "修改描述";
    std::string valText = "按  编辑";
    
    auto keyDim = r->getTextDimensions(keyText, false, keyFont);
    auto valDim = r->getTextDimensions(valText, false, valFont);
    
    s32 keyY = listY + (ITEM_HEIGHT + keyDim.second) / 2;
    s32 valY = listY + (ITEM_HEIGHT + valDim.second) / 2;
    
    r->drawString(keyText, false, x + 19, keyY, keyFont, r->a(tsl::defaultTextColor));
    r->drawString(valText, false, x + w - 15 - valDim.first, valY, valFont, r->a(tsl::onTextColor));
}

bool MacroDetailGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    // 上下键滚动使用说明（参考 updater_ui.cpp L185-196）
    constexpr s32 scrollStep = 8;
    if (keysHeld & HidNpadButton_AnyUp) {
        m_scrollOffset -= scrollStep;
        if (m_scrollOffset < 0) m_scrollOffset = 0;
        return true;
    }
    if (keysHeld & HidNpadButton_AnyDown) {
        m_scrollOffset += scrollStep;
        if (m_scrollOffset > m_maxScrollOffset) m_scrollOffset = m_maxScrollOffset;
        return true;
    }
    
    if (keysDown & HidNpadButton_Plus) {
        u64 tid = MacroUtil::getTitleIdFromPath(m_filePath.c_str());
        char tidStr[17];
        snprintf(tidStr, sizeof(tidStr), "%016lX", tid);
        std::string fileName = ult::getFileName(m_filePath);
        int langIndex = LanguageManager::getZhcnOrZhtwOrEnIndex();
        char editUrl[128];
        snprintf(editUrl, sizeof(editUrl), EDIT_URL, tidStr, fileName.c_str(), langParam[langIndex]);
        tsl::changeTo<MacroInfoEditGui>(editUrl, m_meta.name, m_meta.author);
        return true;
    }
    
    return false;
}
