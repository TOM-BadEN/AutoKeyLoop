#include "macro_upload.hpp"
#include <ultra.hpp>

// 使用说明界面
MacroDetailGui::MacroDetailGui(const char* macroFilePath, const char* gameName) {
    strcpy(m_gameName, gameName);
    strcpy(m_filePath, macroFilePath);
    m_meta = MacroUtil::getMetadata(macroFilePath);
}

tsl::elm::Element* MacroDetailGui::createUI() {
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
            
            // 按 \\n 分割描述文本
            std::string desc = m_meta.desc;
            size_t pos = 0;
            while ((pos = desc.find("\\n")) != std::string::npos) {
                drawWrappedText(desc.substr(0, pos), " • ");
                desc = desc.substr(pos + 2);
            }
            if (!desc.empty()) {
                drawWrappedText(desc, " • ");
            }
        }
    });
    
    frame->setContent(drawer);
    return frame;
}

bool MacroDetailGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_Left || keysDown & HidNpadButton_B) {
        tsl::goBack();
        return true;
    }
    return false;
}
