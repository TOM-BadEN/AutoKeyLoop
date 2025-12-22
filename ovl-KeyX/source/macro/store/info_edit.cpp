#include "info_edit.hpp"
#include "qrcodegen.hpp"

using qrcodegen::QrCode;
using qrcodegen::QrSegment;

MacroInfoEditGui::MacroInfoEditGui(const char* editUrl, const std::string& macroName, const std::string& author)
 : m_editUrl(editUrl)
 , m_macroName(macroName)
 , m_author(author)
 , m_isStoreMacro(!author.empty())
{}

void MacroInfoEditGui::drawNotSupported(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 iconFont = 150;
    s32 textFont = 32;
    s32 gap = 55;
    
    std::string icon = "";
    std::string text = "仅支持下载的脚本";
    
    auto iconDim = r->getTextDimensions(icon, false, iconFont);
    auto textDim = r->getTextDimensions(text, false, textFont);
    
    s32 totalHeight = iconDim.second + gap + textDim.second;
    s32 blockTop = y + (h - totalHeight) / 2;
    
    s32 iconX = x + (w - iconDim.first) / 2;
    s32 iconY = blockTop + iconDim.second;
    
    s32 textX = x + (w - textDim.first) / 2;
    s32 textY = iconY + gap + textDim.second;
    
    r->drawString(icon, false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
    r->drawString(text, false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
}

void MacroInfoEditGui::drawQrCode(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 textX = x + 19;
    s32 currentY = y + 35;
    s32 fontSize = 18;
    s32 lineHeight = 26;
    
    // 大标题：脚本名称 + 作者
    s32 nameFontSize = 28;
    s32 byFontSize = 20;
    std::string byText = " by " + m_author;
    auto nameDim = r->getTextDimensions(m_macroName, false, nameFontSize);
    auto byDim = r->getTextDimensions(byText, false, byFontSize);
    while (nameDim.first + byDim.first > w - 19 - 15) {
        nameFontSize = nameFontSize * 9 / 10;
        byFontSize = byFontSize * 9 / 10;
        nameDim = r->getTextDimensions(m_macroName, false, nameFontSize);
        byDim = r->getTextDimensions(byText, false, byFontSize);
    }
    r->drawString(m_macroName, false, textX, currentY, nameFontSize, r->a(tsl::onTextColor));
    r->drawString(byText, false, textX + nameDim.first, currentY, byFontSize, r->a(tsl::onTextColor));
    currentY += 45;
    
    // 功能介绍
    r->drawString("功能介绍:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    r->drawString(" • 用于修改脚本商店中脚本的信息", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight;
    r->drawString(" • 如脚本名称，使用方法等等", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight + 20;
    
    // 编辑方法
    r->drawString("编辑方法:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    r->drawString(" • 扫描二维码，填写相关信息", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight;
    r->drawString(" • 待审核通过自动生效", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight + 20;
    
    r->drawString("感谢您的热情修改！", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 40;
    
    // 生成二维码（只执行一次）
    if (m_qrSize == 0) {
        QrCode qr = QrCode::encodeSegments(QrSegment::makeSegments(m_editUrl), QrCode::Ecc::LOW, 5, 5);
        m_qrSize = qr.getSize();
        m_qrModules.resize(m_qrSize);
        for (int qy = 0; qy < m_qrSize; qy++) {
            m_qrModules[qy].resize(m_qrSize);
            for (int qx = 0; qx < m_qrSize; qx++) {
                m_qrModules[qy][qx] = qr.getModule(qx, qy);
            }
        }
    }
    
    // 绘制二维码
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

tsl::elm::Element* MacroInfoEditGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("编辑信息", "编辑商店的脚本信息");
    auto list = new tsl::elm::List();
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        if (m_isStoreMacro) drawQrCode(r, x, y, w, h);
        else drawNotSupported(r, x, y, w, h);
    });
    list->addItem(drawer, 520);
    frame->setContent(list);
    return frame;
}
