#include "contribute_ui.hpp"
#include "qrcodegen.hpp"
#include "language.hpp"

using qrcodegen::QrCode;
using qrcodegen::QrSegment;

constexpr const char* GITHUB_URL = "https://github.com/do-kiss/KeyX-Macro-Repository";
constexpr const char* CN_Repo_URL = "https://macro.dokiss.cn/store.php?lang=zh";
constexpr const char* WECHAT_PAY_URL = "wxp://f2f0mMaZS-xnKyAZaTyn813TQRZHTRKPzI6UWxldiWDYemRIq8Stt_LPfd1sLyV4v1jR";
constexpr const char* PAYPAL_URL = "https://www.paypal.com/qrcodes/p2pqrc/7CQ7FTPN26AJ8";

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
        r->drawString(" • 脚本详情界面点击上传按钮", false, textX, currentY, 20, textColor);
        currentY += lineHeight;
        r->drawString(" • 扫描二维码填写脚本信息", false, textX, currentY, 20, textColor);
        currentY += lineHeight;
        r->drawString(" • 待审核通过自动上架商店", false, textX, currentY, 20, textColor);
        currentY += 65;
        
        // 特别感谢
        r->drawString("特别感谢", false, textX, currentY, 24, titleColor);
        currentY += 34;
        r->drawString(" • 忘忧 (dokiss.cn)", false, textX, currentY, 20, textColor);
        currentY += lineHeight;
        r->drawString(" • 感谢提供服务器与制作网页", false, textX, currentY, 20, textColor);
        currentY += 65;
        
        // 两个二维码 - 根据语言只生成需要的2个
        static bool isChinese = LanguageManager::isSimplifiedChinese();
        static QrCode qrRepo = QrCode::encodeSegments(QrSegment::makeSegments(isChinese ? CN_Repo_URL : GITHUB_URL), QrCode::Ecc::LOW, 5, 5);
        static QrCode qrDonate = QrCode::encodeSegments(QrSegment::makeSegments(isChinese ? WECHAT_PAY_URL : PAYPAL_URL), QrCode::Ecc::LOW, 5, 5);
        const char* donateTitle = isChinese ? "微信捐赠" : "PayPal Donate";
        
        int scale = 3;
        int margin = 6;
        int qrSizeRepo = qrRepo.getSize();
        int qrSizeDonate = qrDonate.getSize();
        int totalSizeRepo = qrSizeRepo * scale;
        int totalSizeDonate = qrSizeDonate * scale;
        int startXRepo = textX + 15;
        int startXDonate = x + w - 20 - totalSizeDonate - 15;
        
         tsl::Color donateColor = {0x66, 0xFF, 0x66, 0xFF};  // 绿色
        // 双标题：脚本仓库（左，居中于二维码）  捐赠（右，绿色，居中于二维码）
        auto [repoW, repoH] = r->getTextDimensions("脚本仓库", false, 24);
        s32 repoTitleX = startXRepo + (totalSizeRepo - repoW) / 2;
        r->drawString("脚本仓库", false, repoTitleX, currentY, 24, donateColor);
        
        auto [donateW, donateH] = r->getTextDimensions(donateTitle, false, 24);
        s32 donateTitleX = startXDonate + (totalSizeDonate - donateW) / 2;
       
        r->drawString(donateTitle, false, donateTitleX, currentY, 24, donateColor);
        currentY += 34;
        
        // 脚本仓库二维码（左）
        int startYRepo = currentY;
        
        r->drawRect(startXRepo - margin, startYRepo - margin, totalSizeRepo + margin * 2, totalSizeRepo + margin * 2, tsl::Color(0xF, 0xF, 0xF, 0xF));
        for (int qy = 0; qy < qrSizeRepo; qy++) {
            for (int qx = 0; qx < qrSizeRepo; qx++) {
                if (qrRepo.getModule(qx, qy)) {
                    r->drawRect(startXRepo + qx * scale, startYRepo + qy * scale, scale, scale, tsl::Color(0x0, 0x0, 0x0, 0xF));
                }
            }
        }
        
        // 捐赠二维码（右）
        int startYDonate = currentY;
        
        r->drawRect(startXDonate - margin, startYDonate - margin, totalSizeDonate + margin * 2, totalSizeDonate + margin * 2, tsl::Color(0xF, 0xF, 0xF, 0xF));
        for (int qy = 0; qy < qrSizeDonate; qy++) {
            for (int qx = 0; qx < qrSizeDonate; qx++) {
                if (qrDonate.getModule(qx, qy)) {
                    r->drawRect(startXDonate + qx * scale, startYDonate + qy * scale, scale, scale, tsl::Color(0x0, 0x0, 0x0, 0xF));
                }
            }
        }
    });
    
    list->addItem(contentDrawer, 520);
    frame->setContent(list);
    return frame;
}
