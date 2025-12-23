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

    constexpr const char* UPLOAD_URL = "https://macro.dokiss.cn/edit.php?code=%s&lang=%s";
    constexpr const char* EDIT_URL = "https://macro.dokiss.cn/edit_prop.php?tid=%s&file=%s&lang=%s";

    constexpr const char* langParam[] = {"zh", "zhh", "en"};
}

// 脚本详情界面
MacroUploadGui::MacroUploadGui(const std::string& macroFilePath, const std::string& gameName) 
 : m_gameName(gameName)
{
    m_meta = MacroUtil::getMetadata(macroFilePath);
    u64 titleId = MacroUtil::getTitleIdFromPath(macroFilePath.c_str());
    std::string filePath = macroFilePath;
    Thd::start([this, titleId, filePath]{ m_uploadResult = StoreData::uploadMacro(filePath, titleId, m_gameName); });
}

MacroUploadGui::~MacroUploadGui() {
    ult::abortDownload = true;
    Thd::stop();
}

tsl::elm::Element* MacroUploadGui::createUI() {
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    frame->setHeader(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString(m_gameName, false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString("将脚本投稿至脚本商店", false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
    }));
    auto list = new tsl::elm::List();
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        switch (m_state) {
            case UploadState::UploadSuccess:
                drawUploadSuccess(r, x, y, w, h);
                break;
            default:
                drawUploading(r, x, y, w, h);
                break;
        }
    });
    list->addItem(drawer, 520);
    frame->setContent(list);
    return frame;
}

void MacroUploadGui::drawUploading(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 iconFont = 150;
    s32 textFont = 32;
    s32 gap = 55;
    
    if (m_state == UploadState::Uploading) {
        // 上传中：动画图标 + 文字
        auto iconDim = r->getTextDimensions("", false, iconFont);
        auto textDim = r->getTextDimensions("正在上传脚本", false, textFont);
        
        s32 totalHeight = iconDim.second + gap + textDim.second;
        s32 blockTop = y + (h - totalHeight) / 2;
        
        s32 iconX = x + (w - iconDim.first) / 2;
        s32 iconY = blockTop + iconDim.second;
        
        s32 textX = x + (w - textDim.first) / 2;
        s32 textY = iconY + gap + textDim.second;
        
        r->drawString(refreshIcon[m_frameIndex], false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
        r->drawString("正在上传脚本", false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));

    } else if (m_state == UploadState::UploadFailed) {
        // 上传失败：错误图标 + 两行提示
        auto iconDim = r->getTextDimensions("", false, iconFont);
        auto textDim = r->getTextDimensions("请检查网络连接", false, textFont);
        auto hintDim = r->getTextDimensions("请检查时间同步", false, textFont);
        
        s32 totalHeight = iconDim.second + gap + textDim.second + gap / 2 + hintDim.second;
        s32 blockTop = y + (h - totalHeight) / 2;
        
        s32 iconX = x + (w - iconDim.first) / 2;
        s32 iconY = blockTop + iconDim.second;
        
        s32 textX = x + (w - textDim.first) / 2;
        s32 textY = iconY + gap + textDim.second;
        
        s32 hintX = x + (w - hintDim.first) / 2;
        s32 hintY = textY + gap / 2 + hintDim.second;
        
        r->drawString("", false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
        r->drawString("请检查网络连接", false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
        r->drawString("请检查时间同步", false, hintX, hintY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
    }
}

void MacroUploadGui::drawUploadSuccess(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
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

void MacroUploadGui::prepareSuccessData() {
    // 生成二维码
    int langIndex = LanguageManager::getZhcnOrZhtwOrEnIndex();
    char uploadUrl[96];
    snprintf(uploadUrl, sizeof(uploadUrl), UPLOAD_URL, m_uploadResult.code.c_str(), langParam[langIndex]);
    QrCode qr = QrCode::encodeSegments(QrSegment::makeSegments(uploadUrl), QrCode::Ecc::LOW, 5, 5);
    m_qrSize = qr.getSize();
    m_qrModules.resize(m_qrSize);
    for (int qy = 0; qy < m_qrSize; qy++) {
        m_qrModules[qy].resize(m_qrSize);
        for (int qx = 0; qx < m_qrSize; qx++) {
            m_qrModules[qy][qx] = qr.getModule(qx, qy);
        }
    }
}

void MacroUploadGui::update() {
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

bool MacroUploadGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    
    // 其他状态：按 B 返回
    if (keysDown & HidNpadButton_B) {
        ult::abortDownload = true;
        Thd::stop();
        tsl::goBack();
        return true;
    }
    
    return false;
}
