#include "updater_ui.hpp"
#include <string>
#include <vector>
#include <cmath>

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

// 静态成员定义
alignas(0x1000) char UpdaterUI::s_threadStack[32 * 1024];

UpdaterUI::UpdaterUI() {
    m_state = UpdateState::GettingJson;
    startThread();    // 启动后台线程,这里是下载updateJSON    
}

UpdaterUI::~UpdaterUI() {
    ult::abortDownload = true;    // 通知 curl 取消下载
    stopThread();     // 确保线程被正确清理
}

tsl::elm::Element* UpdaterUI::createUI() {
    auto frame = new tsl::elm::OverlayFrame("检查更新", APP_VERSION_STRING);
    auto list = new tsl::elm::List();
    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        switch (m_state) {
            case UpdateState::GettingJson:  drawGettingJson(r, x, y, w, h); break;
            case UpdateState::NetworkError: drawNetworkError(r, x, y, w, h); break;
            case UpdateState::NoUpdate:     drawNoUpdate(r, x, y, w, h); break;
            case UpdateState::HasUpdate:    drawHasUpdate(r, x, y, w, h); break;
        }
    });
    list->addItem(textArea, 520);
    frame->setContent(list);
    return frame;
}

void UpdaterUI::update() {
    if (m_state == UpdateState::GettingJson) {
        m_frameCounter++;
        if (m_frameCounter >= 12) {
            m_frameCounter = 0;
            m_frameIndex = (m_frameIndex + 1) % 8;
        }
        
        if (m_taskDone) {
            stopThread();
            if (!m_updateInfo.success) {
                m_state = UpdateState::NetworkError;
            } else {
                bool hasNew = m_updateData.hasNewVersion(m_updateInfo.version, APP_VERSION_STRING);
                m_state = hasNew ? UpdateState::HasUpdate : UpdateState::NoUpdate;
            }
        }
        
    }
}


void UpdaterUI::drawGettingJson(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 iconFont = 150;
    s32 textFont = 32;
    s32 gap = 55;
    
    // 获取尺寸
    auto iconDim = r->getTextDimensions("", false, iconFont);  // 刷新图标
    auto textDim = r->getTextDimensions("检查更新中", false, textFont);
    
    // 计算垂直居中
    s32 totalHeight = iconDim.second + gap + textDim.second;
    s32 blockTop = y + (h - totalHeight) / 2;
    
    // 图标居中
    s32 iconX = x + (w - iconDim.first) / 2;
    s32 iconY = blockTop + iconDim.second;
    
    // 文字居中
    s32 textX = x + (w - textDim.first) / 2;
    s32 textY = iconY + gap + textDim.second;
    
    r->drawString(refreshIcon[m_frameIndex], false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
    r->drawString("检查更新中", false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
}

void UpdaterUI::drawNetworkError(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 iconFont = 150;
    s32 textFont = 32;
    s32 gap = 55;
    
    auto iconDim = r->getTextDimensions("", false, iconFont);
    auto textDim = r->getTextDimensions(m_updateInfo.error, false, textFont);
    
    s32 totalHeight = iconDim.second + gap + textDim.second;
    s32 blockTop = y + (h - totalHeight) / 2;
    
    s32 iconX = x + (w - iconDim.first) / 2;
    s32 iconY = blockTop + iconDim.second;
    
    s32 textX = x + (w - textDim.first) / 2;
    s32 textY = iconY + gap + textDim.second;
    
    r->drawString("", false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
    r->drawString(m_updateInfo.error, false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
}

void UpdaterUI::drawNoUpdate(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 iconFont = 150;
    s32 textFont = 32;
    s32 gap = 55;
    
    auto iconDim = r->getTextDimensions("", false, iconFont);
    auto textDim = r->getTextDimensions("已是最新版本", false, textFont);
    
    s32 totalHeight = iconDim.second + gap + textDim.second;
    s32 blockTop = y + (h - totalHeight) / 2;
    
    s32 iconX = x + (w - iconDim.first) / 2;
    s32 iconY = blockTop + iconDim.second;
    
    s32 textX = x + (w - textDim.first) / 2;
    s32 textY = iconY + gap + textDim.second;
    
    r->drawString("", false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
    r->drawString("已是最新版本", false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
}

void UpdaterUI::drawHasUpdate(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 textX = x + 19;
    s32 currentY = y + 35;
    
    // 发现新版本 • v1.4.2 (青色，大字号)
    std::string versionText = "发现新版本 " + m_updateInfo.version;
    r->drawString(versionText, false, textX, currentY, 28, r->a(tsl::onTextColor));
    currentY += 50;
    
    // 更新内容
    r->drawString("更新内容 :", false, textX, currentY, 23, r->a(tsl::defaultTextColor));
    currentY += 35;
    
    // changelog 列表（自动换行）
    s32 maxWidth = w - 19 - 15;
    s32 fontSize = 20;
    s32 lineHeight = 32;
    s32 listY = y + h - 10 - ITEM_HEIGHT;
    s32 maxY = listY - 20;
    bool stopDrawing = false;
    
    for (const auto& item : m_updateInfo.changelog) {
        if (stopDrawing) break;
        std::string text = item;
        std::string prefix = " • ";
        bool isFirstLine = true;
        
        while (!text.empty()) {
            if (currentY > maxY) { stopDrawing = true; break; }
            
            std::string tryLine = prefix + text;
            auto [tw, th] = r->getTextDimensions(tryLine, false, fontSize);
            s32 drawX = isFirstLine ? textX : textX + 2;
            
            if (tw <= maxWidth) {
                r->drawString(tryLine, false, drawX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
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
            
            // 二分查找截断点（按字符数）
            size_t lo = 1, hi = charBounds.size() - 1, cutIdx = 1;
            while (lo <= hi) {
                size_t mid = (lo + hi) / 2;
                auto [mw, mh] = r->getTextDimensions(prefix + text.substr(0, charBounds[mid]), false, fontSize);
                if (mw <= maxWidth) {
                    cutIdx = mid;
                    lo = mid + 1;
                } else {
                    hi = mid - 1;
                }
            }
            
            size_t cut = charBounds[cutIdx];
            r->drawString(prefix + text.substr(0, cut), false, drawX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
            currentY += lineHeight;
            text = text.substr(cut);
            prefix = "   ";
            isFirstLine = false;
        }
    }
    
    // 绘制底部按钮
    double progress = calcBlinkProgress();
    tsl::Color hlColor = calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
    
    // 绘制选中框
    r->drawBorderedRoundedRect(x, listY, w + 4, ITEM_HEIGHT, 5, 5, r->a(hlColor));
    
    // 绘制文字
    s32 keyFont = 23;
    s32 valFont = 20;
    auto keyDim = r->getTextDimensions("更新插件", false, keyFont);
    auto valDim = r->getTextDimensions("按+更新", false, valFont);
    
    s32 keyY = listY + (ITEM_HEIGHT + keyDim.second) / 2;
    s32 valY = listY + (ITEM_HEIGHT + valDim.second) / 2;
    
    r->drawString("更新插件", false, x + 19, keyY, keyFont, r->a(tsl::defaultTextColor));
    r->drawString("按+更新", false, x + w - 15 - valDim.first, valY, valFont, r->a(tsl::onTextColor));
}

// 线程函数
void UpdaterUI::ThreadFunc(void* arg) {
    UpdaterUI* self = static_cast<UpdaterUI*>(arg);
    if (self->m_state == UpdateState::GettingJson) self->m_updateInfo = self->m_updateData.getUpdateInfo();
    self->m_taskDone = true;
}

// 启动线程
void UpdaterUI::startThread() {
    m_taskDone = false;
    m_threadCreated = false;
    memset(&m_thread, 0, sizeof(Thread));
    Result rc = threadCreate(&m_thread, ThreadFunc, this, s_threadStack, sizeof(s_threadStack), 0x2C, -2);
    if (R_SUCCEEDED(rc)) {
        m_threadCreated = true;
        threadStart(&m_thread);
    }
}

// 停止线程
void UpdaterUI::stopThread() {
    if (m_threadCreated) {
        threadWaitForExit(&m_thread);
        threadClose(&m_thread);
        m_threadCreated = false;
    }
}
