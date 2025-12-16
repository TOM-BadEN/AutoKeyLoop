#include "contribute_ui.hpp"
#include "qrcodegen.hpp"

using qrcodegen::QrCode;

static const char* GITHUB_URL = "https://github.com/TOM-BadEN/KeyX-Macro-Repo";
static const char* AFDIAN_URL = "https://afdian.com/a/TOM-BadEN?from=keyx";

ContributeGui::ContributeGui() {}

tsl::elm::Element* ContributeGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("我要投稿", "欢迎向我提供各种脚本");
    auto list = new tsl::elm::List();

    auto contentDrawer = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        s32 textX = x + 20;
        s32 currentY = y + 45;
        s32 lineHeight = 25;
        tsl::Color titleColor = {0x66, 0xCC, 0xFF, 0xFF};  // 亮蓝色
        tsl::Color textColor = {0xFF, 0xFF, 0xFF, 0xFF};   // 白色
        
        // 投稿方式
        r->drawString("投稿方式", false, textX, currentY, 24, titleColor);
        currentY += 34;
        r->drawString(" • 通过社交软件直接向我投稿", false, textX, currentY, 20, textColor);
        currentY += lineHeight;
        r->drawString(" • 只需要提供宏文件和说明即可", false, textX, currentY, 20, textColor);
        currentY += lineHeight;
        r->drawString(" • 向我的 GitHub 仓库提交 PR", false, textX, currentY, 20, textColor);
        currentY += 65;
        
        // 联系方式
        r->drawString("联系方式", false, textX, currentY, 24, titleColor);
        currentY += 34;
        r->drawString(" • QQ群: 1051287661", false, textX, currentY, 20, textColor);
        currentY += lineHeight;
        r->drawString(" • DC: phenomenal_tiger_48068", false, textX, currentY, 20, textColor);
        currentY += 65;
        
        // 两个二维码 - 先计算位置
        static QrCode qrGithub = QrCode::encodeText(GITHUB_URL, QrCode::Ecc::LOW);
        static QrCode qrAfdian = QrCode::encodeText(AFDIAN_URL, QrCode::Ecc::LOW);
        
        int scale = 4;
        int margin = 6;
        int qrSizeGithub = qrGithub.getSize();
        int qrSizeAfdian = qrAfdian.getSize();
        int totalSizeGithub = qrSizeGithub * scale;
        int totalSizeAfdian = qrSizeAfdian * scale;
        int startXGithub = textX + 15;
        int startXAfdian = x + w - 20 - totalSizeAfdian - 15;
        
        // 双标题：GitHub 仓库（左）  向我捐赠（右，绿色，居中于二维码）
        r->drawString("GitHub 仓库", false, textX, currentY, 24, titleColor);
        
        auto [donateW, donateH] = r->getTextDimensions("向我捐赠", false, 24);
        s32 donateTitleX = startXAfdian + (totalSizeAfdian - donateW) / 2;
        tsl::Color donateColor = {0x66, 0xFF, 0x66, 0xFF};  // 绿色
        r->drawString("向我捐赠", false, donateTitleX, currentY, 24, donateColor);
        currentY += 34;
        
        // GitHub 二维码（左）
        int startYGithub = currentY;
        
        r->drawRect(startXGithub - margin, startYGithub - margin, totalSizeGithub + margin * 2, totalSizeGithub + margin * 2, tsl::Color(0xF, 0xF, 0xF, 0xF));
        for (int qy = 0; qy < qrSizeGithub; qy++) {
            for (int qx = 0; qx < qrSizeGithub; qx++) {
                if (qrGithub.getModule(qx, qy)) {
                    r->drawRect(startXGithub + qx * scale, startYGithub + qy * scale, scale, scale, tsl::Color(0x0, 0x0, 0x0, 0xF));
                }
            }
        }
        
        // 爱发电二维码（右）
        int startYAfdian = currentY;
        
        r->drawRect(startXAfdian - margin, startYAfdian - margin, totalSizeAfdian + margin * 2, totalSizeAfdian + margin * 2, tsl::Color(0xF, 0xF, 0xF, 0xF));
        for (int qy = 0; qy < qrSizeAfdian; qy++) {
            for (int qx = 0; qx < qrSizeAfdian; qx++) {
                if (qrAfdian.getModule(qx, qy)) {
                    r->drawRect(startXAfdian + qx * scale, startYAfdian + qy * scale, scale, scale, tsl::Color(0x0, 0x0, 0x0, 0xF));
                }
            }
        }
    });
    
    list->addItem(contentDrawer, 520);
    frame->setContent(list);
    return frame;
}
