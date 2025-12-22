#include "macro_upload.hpp"
#include <ultra.hpp>
#include <cmath>
#include "Tthread.hpp"
#include "qrcodegen.hpp"
#include "language.hpp"

using qrcodegen::QrCode;
using qrcodegen::QrSegment;

namespace {
    constexpr const char* refreshIcon[] = {"", "", "", "", "", "", "", ""};
    constexpr s32 ITEM_HEIGHT = 70;
    
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
MacroDetailGui::MacroDetailGui(const char* macroFilePath, const char* gameName) {
    strncpy(m_gameName, gameName, sizeof(m_gameName) - 1);
    m_gameName[sizeof(m_gameName) - 1] = '\0';
    strcpy(m_filePath, macroFilePath);
    m_meta = MacroUtil::getMetadata(macroFilePath);
}

tsl::elm::Element* MacroDetailGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame(m_gameName, "脚本的相关介绍与上传");
    auto list = new tsl::elm::List();
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        switch (m_state) {
            case UploadState::UploadSuccess:
                drawUploadSuccess(r, x, y, w, h);
                break;
            default:
                drawDetail(r, x, y, w, h);
                break;
        }
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
    
    // ========== 底部按钮 ==========
    // 绘制选中框
    double progress = calcBlinkProgress();
    tsl::Color hlColor = calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
    r->drawBorderedRoundedRect(x, listY, w + 4, ITEM_HEIGHT, 5, 5, r->a(hlColor));
    
    // 根据状态显示不同内容
    s32 keyFont = 23;
    s32 valFont = 20;
    std::string keyText, valText;
    tsl::Color keyColor = tsl::defaultTextColor;
    
    switch (m_state) {
        case UploadState::Uploading:
            keyText = "我要上传";
            valText = refreshIcon[m_frameIndex];
            break;
        case UploadState::UploadFailed:
            keyText = "上传失败";
            valText = "按  上传";
            keyColor = tsl::Color(0xF, 0x5, 0x5, 0xF);  // 红色
            break;
        case UploadState::Cancelled:
            keyText = "取消成功";
            valText = "按  上传";
            break;
        default:  // None
            keyText = "我要上传";
            valText = "按  上传";
            break;
    }
    
    auto keyDim = r->getTextDimensions(keyText, false, keyFont);
    auto valDim = r->getTextDimensions(valText, false, valFont);
    
    s32 keyY = listY + (ITEM_HEIGHT + keyDim.second) / 2;
    s32 valY = listY + (ITEM_HEIGHT + valDim.second) / 2;
    
    r->drawString(keyText, false, x + 19, keyY, keyFont, r->a(keyColor));
    r->drawString(valText, false, x + w - 15 - valDim.first, valY, valFont, r->a(tsl::onTextColor));
}

void MacroDetailGui::drawUploadSuccess(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 textX = x + 19;
    s32 currentY = y + 35;
    s32 fontSize = 18;
    s32 lineHeight = 26;
    
    // 上传成功（标题，与 drawDetail 的脚本名称样式一致）
    r->drawString("最后一步了~", false, textX, currentY, 28, r->a(tsl::onTextColor));
    currentY += 50;
    
    // 脚本代码：
    r->drawString("脚本代码:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    r->drawString((" • " + m_uploadResult.code).c_str(), false, textX, currentY, fontSize, r->a(tsl::onTextColor));
    currentY += lineHeight + 20;
    
    // 填写信息：
    r->drawString("填写信息:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    r->drawString(" • 扫描二维码，填写宏的相关信息", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight;
    r->drawString(" • 待审核通过自动上架商店", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight + 20;

    r->drawString("感谢您的热情分享！", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 40;
    
    // 绘制二维码（使用缓存的 m_qrModules）
    int scale = 5;
    int margin = 8;
    int totalSize = m_qrSize * scale;
    int qrX = x + (w - totalSize) / 2;
    int qrY = currentY;
    
    // 绘制二维码背景
    r->drawRect(qrX - margin, qrY - margin, totalSize + margin * 2, totalSize + margin * 2, tsl::Color(0xF, 0xF, 0xF, 0xF));
    
    // 绘制二维码
    for (int qy = 0; qy < m_qrSize; qy++) {
        for (int qx = 0; qx < m_qrSize; qx++) {
            if (m_qrModules[qy][qx]) {
                r->drawRect(qrX + qx * scale, qrY + qy * scale, scale, scale, tsl::Color(0x0, 0x0, 0x0, 0xF));
            }
        }
    }
}

void MacroDetailGui::prepareSuccessData() {
    // 生成二维码
    const char* langParam[] = {"zh", "zhh", "en"};
    int langIndex = LanguageManager::getZhcnOrZhtwOrEnIndex();
    std::string qrUrl = "https://macro.dokiss.cn/edit.php?code=" + m_uploadResult.code + "&lang=" + langParam[langIndex];
    QrCode qr = QrCode::encodeSegments(QrSegment::makeSegments(qrUrl.c_str()), QrCode::Ecc::LOW, 5, 5);
    m_qrSize = qr.getSize();
    m_qrModules.resize(m_qrSize);
    for (int qy = 0; qy < m_qrSize; qy++) {
        m_qrModules[qy].resize(m_qrSize);
        for (int qx = 0; qx < m_qrSize; qx++) {
            m_qrModules[qy][qx] = qr.getModule(qx, qy);
        }
    }
}

void MacroDetailGui::update() {
    if (m_state == UploadState::Uploading) {
        m_frameCounter++;
        if (m_frameCounter >= 12) {
            m_frameCounter = 0;
            m_frameIndex = (m_frameIndex + 1) % 8;
        }
        
        // 独立线程执行完毕
        if (Thd::isDone()) Thd::stop();
        else return;

        // 检查是否成功了
        if (!m_uploadResult.success) {
            m_state = UploadState::UploadFailed;
            return;
        }
        // 获取二维码数据和解析json
        prepareSuccessData();
        m_state = UploadState::UploadSuccess;
    }
}

bool MacroDetailGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    // Uploading 状态：按 B 取消上传
    if (m_state == UploadState::Uploading) {
        if (keysDown & HidNpadButton_B) {
            ult::abortDownload = true;
            Thd::stop();
            m_state = UploadState::Cancelled;
            return true;
        }
    }
    
    // 其他状态：按 B 返回
    if (keysDown & HidNpadButton_B) {
        ult::abortDownload = true;
        Thd::stop();
        tsl::goBack();
        return true;
    }
    
    // None/UploadFailed/Cancelled 状态：按 + 触发上传
    if (m_state == UploadState::None || m_state == UploadState::UploadFailed || m_state == UploadState::Cancelled) {
        if (keysDown & HidNpadButton_Plus) {
            m_state = UploadState::Uploading;
            u64 titleId = MacroUtil::getTitleIdFromPath(m_filePath);
            Thd::start([this, titleId]{ m_uploadResult = StoreData::uploadMacro(m_filePath, titleId, m_gameName); });
            return true;
        }
    }
    
    return false;
}
