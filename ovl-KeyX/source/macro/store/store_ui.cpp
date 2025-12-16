#include "store_ui.hpp"

StoreGui::StoreGui() {}

tsl::elm::Element* StoreGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("脚本商店", "敬请期待");
    auto list = new tsl::elm::List();

    auto contentDrawer = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        s32 textX = x + 20;
        s32 currentY = y + 40;
        s32 lineHeight = 22;
        tsl::Color titleColor = {0xFF, 0xCC, 0x00, 0xFF};  // 黄色警告色
        tsl::Color textColor = {0xFF, 0xFF, 0xFF, 0xFF};   // 白色
        tsl::Color highlightColor = {0x66, 0xFF, 0x66, 0xFF};  // 绿色
        
        r->drawString("版本说明", false, textX, currentY, 20, titleColor);
        currentY += 28;
        r->drawString("1.4.2 引入网络功能，使用默认套接字参数", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("在 Ultrahand 设置为 4MB ovlloader 时", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("会因内存不足，引发系统崩溃", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("为修复该问题，先发布 1.4.3", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("脚本商店将在 1.4.4 版本上线", false, textX, currentY, 18, highlightColor);
        currentY += lineHeight * 2;
        
        // 英文说明
        r->drawString("Version Notes", false, textX, currentY, 20, titleColor);
        currentY += 28;
        r->drawString("v1.4.2 introduced network feature", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("using default socket parameters", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("On Ultrahand with", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("4MB ovlloader setting", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("it causes system crash", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("due to insufficient memory", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("v1.4.3 released to fix the crash", false, textX, currentY, 18, textColor);
        currentY += lineHeight;
        r->drawString("Macro Store coming in v1.4.4", false, textX, currentY, 18, highlightColor);
    });
    
    list->addItem(contentDrawer, 520);
    frame->setContent(list);
    return frame;
}
