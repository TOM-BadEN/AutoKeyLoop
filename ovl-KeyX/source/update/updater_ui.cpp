#include "updater_ui.hpp"
#include <string>
#include <vector>
#include <cmath>
#include "sysmodule.hpp"
#include <ultra.hpp>
#include "i18n.hpp"
#include "Tthread.hpp"

namespace {
    constexpr const char* refreshIcon[] = {"", "", "", "", "", "", "", ""};
    constexpr s32 ITEM_HEIGHT = 70;
    
    // changelog 布局常量
    constexpr s32 CHANGELOG_FONT_SIZE = 20;    // 字体大小
    constexpr s32 CHANGELOG_LINE_HEIGHT = 32;  // 行高
    constexpr s32 CHANGELOG_ITEM_GAP = 3;      // 条目间隙
    constexpr s32 SCROLLBAR_WIDTH = 3;         // 滚动条宽度
    constexpr s32 SCROLLBAR_GAP = 8;           // 文字到滚动条间隙
    
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
    
    // changelog 行数据
    struct ChangelogLine {
        std::string text;
        bool hasPrefix;  // 是否显示 " • " 前缀
    };
    
    // 预处理 changelog 为行数据
    std::vector<ChangelogLine> preprocessChangelog(
        tsl::gfx::Renderer* r, const std::vector<std::string>& changelog,
        s32 maxWidth, s32 prefixW, s32& outTotalHeight) {
        
        std::vector<ChangelogLine> lines;
        outTotalHeight = 0;
        
        for (const auto& item : changelog) {
            std::string text = item;
            bool isFirstLine = true;
            while (!text.empty()) {
                s32 lineMaxWidth = isFirstLine ? maxWidth : (maxWidth - prefixW);
                auto [tw, th] = r->getTextDimensions(text, false, CHANGELOG_FONT_SIZE);
                
                if (tw <= lineMaxWidth) {
                    lines.push_back({text, isFirstLine});
                    outTotalHeight += CHANGELOG_LINE_HEIGHT;
                    break;
                }
                
                auto bounds = getUtf8CharBounds(text);
                size_t cut = findTextCutPoint(r, text, bounds, lineMaxWidth, CHANGELOG_FONT_SIZE);
                lines.push_back({text.substr(0, cut), isFirstLine});
                outTotalHeight += CHANGELOG_LINE_HEIGHT;
                text = text.substr(cut);
                isFirstLine = false;
            }
            outTotalHeight += CHANGELOG_ITEM_GAP;
        }
        return lines;
    }
}


UpdaterUI::UpdaterUI() {
    m_state = UpdateState::GettingJson;
    Thd::start([this]{ m_updateInfo = m_updateData.getUpdateInfo();});
}

UpdaterUI::~UpdaterUI() {
    ult::abortDownload = true;    // 通知 curl 取消下载
    ult::abortUnzip = true;       // 通知 unzip 取消解压
    Thd::stop();
}

tsl::elm::Element* UpdaterUI::createUI() {
    auto frame = new tsl::elm::OverlayFrame("检查更新", APP_VERSION_STRING);
    auto list = new tsl::elm::List();
    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        switch (m_state) {
            case UpdateState::GettingJson:  
                drawGettingJson(r, x, y, w, h); 
                break;
            case UpdateState::NetworkError: 
                drawNetworkError(r, x, y, w, h); 
                break;
            case UpdateState::UpdateSuccess: 
            case UpdateState::NoUpdate:     
                drawNoUpdate(r, x, y, w, h); 
                break;
            case UpdateState::HasUpdate:
            case UpdateState::Downloading:
            case UpdateState::Unzipping:
            case UpdateState::CancelUpdate:
            case UpdateState::UpdateError:
                drawHasUpdate(r, x, y, w, h); 
                break;
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
        
        if (Thd::isDone()) {
            Thd::stop();
            if (!m_updateInfo.success) {
                m_state = UpdateState::NetworkError;
            } else {
                bool hasNew = m_updateData.hasNewVersion(m_updateInfo.version, APP_VERSION_STRING);
                m_state = hasNew ? UpdateState::HasUpdate : UpdateState::NoUpdate;
            }
        }
        
    }

    else if (m_state == UpdateState::Downloading && Thd::isDone()) {
        Thd::stop();
        if (m_successDownload) {
            m_state = UpdateState::Unzipping;
            Thd::start([this]{ m_successUnzip = m_updateData.unzipZip();});
        } else {
            m_state = UpdateState::UpdateError;
        }    
    }
    
    else if (m_state == UpdateState::Unzipping && Thd::isDone()) {
        Thd::stop();
        if (m_successUnzip) {
            SysModuleManager::restartModule();
            m_state = UpdateState::UpdateSuccess;
        } else {
            m_state = UpdateState::UpdateError;
        }
    }
}

bool UpdaterUI::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (m_state == UpdateState::HasUpdate || m_state == UpdateState::CancelUpdate || m_state == UpdateState::UpdateError) {
        // 上下键滚动 changelog
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
            m_state = UpdateState::Downloading;
            Thd::start([this]{ m_successDownload = m_updateData.downloadZip();});
            return true;
        }
    }

    if (m_state == UpdateState::Downloading || m_state == UpdateState::Unzipping) {
        if (keysDown & HidNpadButton_B) {
            ult::abortDownload = true;
            ult::abortUnzip = true;
            Thd::stop();
            m_state = UpdateState::CancelUpdate;
            return true;
        }
    }
    return false;
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
    
    bool isSuccess = (m_state == UpdateState::UpdateSuccess);
    
    auto iconDim = r->getTextDimensions("", false, iconFont);
    auto textDim = r->getTextDimensions("已是最新版本", false, textFont);
    auto hintDim = r->getTextDimensions("重启按键助手生效", false, textFont);
    
    s32 totalHeight = iconDim.second + gap + textDim.second;
    if (isSuccess) totalHeight += gap / 2 + hintDim.second;
    
    s32 blockTop = y + (h - totalHeight) / 2;
    
    s32 iconX = x + (w - iconDim.first) / 2;
    s32 iconY = blockTop + iconDim.second;
    
    s32 textX = x + (w - textDim.first) / 2;
    s32 textY = iconY + gap + textDim.second;
    
    r->drawString("", false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
    r->drawString("已是最新版本", false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
    
    if (isSuccess) {
        s32 hintX = x + (w - hintDim.first) / 2;
        s32 hintY = textY + gap / 2 + hintDim.second;
        r->drawString("重启按键助手生效", false, hintX, hintY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
    }
}

void UpdaterUI::drawHasUpdate(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    // 布局参数
    s32 textX = x + 19;
    s32 currentY = y + 35;
    s32 listY = y + h - 10 - ITEM_HEIGHT;
    s32 maxWidth = w - 19 - 15 - SCROLLBAR_WIDTH - SCROLLBAR_GAP;
    
    // 绘制标题
    std::string versionText = i18n("发现新版本 ") + m_updateInfo.version;
    r->drawString(versionText, false, textX, currentY, 28, r->a(tsl::onTextColor));
    currentY += 50;
    
    r->drawString("更新内容 :", false, textX, currentY, 23, r->a(tsl::defaultTextColor));
    currentY += 35;
    
    // 计算区域边界
    s32 textMinY = currentY;
    s32 maxY = listY - 22;
    s32 visibleHeight = maxY - textMinY;
    s32 scrollMinY = textMinY - CHANGELOG_LINE_HEIGHT + 5;
    
    // 预处理 changelog
    std::string prefix = " • ";
    auto [prefixW, prefixH] = r->getTextDimensions(prefix, false, CHANGELOG_FONT_SIZE);
    s32 totalContentHeight = 0;
    auto lines = preprocessChangelog(r, m_updateInfo.changelog, maxWidth, prefixW, totalContentHeight);
    
    // 更新滚动状态
    m_maxScrollOffset = (totalContentHeight > visibleHeight) ? (totalContentHeight - visibleHeight) : 0;
    if (m_scrollOffset > m_maxScrollOffset) m_scrollOffset = m_maxScrollOffset;
    if (m_scrollOffset < 0) m_scrollOffset = 0;
    
    // 绘制 changelog
    s32 maxLines = (visibleHeight + CHANGELOG_LINE_HEIGHT - 1) / CHANGELOG_LINE_HEIGHT;
    s32 contentY = 0, drawY = textMinY, drawnLines = 0;
    
    for (const auto& line : lines) {
        if (drawnLines >= maxLines) break;
        if (contentY >= m_scrollOffset) {
            if (line.hasPrefix) {
                r->drawString(prefix + line.text, false, textX, drawY, CHANGELOG_FONT_SIZE, r->a(tsl::style::color::ColorDescription));
            } else {
                r->drawString(line.text, false, textX + prefixW, drawY, CHANGELOG_FONT_SIZE, r->a(tsl::style::color::ColorDescription));
            }
            drawY += CHANGELOG_LINE_HEIGHT;
            drawnLines++;
        }
        contentY += CHANGELOG_LINE_HEIGHT;
    }
    
    // 绘制滚动条
    if (m_maxScrollOffset > 0) {
        s32 scrollBarX = x + w - SCROLLBAR_WIDTH;
        s32 scrollBarTotalH = maxY - scrollMinY;
        s32 scrollBarHeight = scrollBarTotalH * visibleHeight / totalContentHeight;
        if (scrollBarHeight < 20) scrollBarHeight = 20;
        s32 scrollBarY = scrollMinY + (scrollBarTotalH - scrollBarHeight) * m_scrollOffset / m_maxScrollOffset;
        s32 radius = SCROLLBAR_WIDTH / 2;
        
        r->drawRoundedRect(scrollBarX, scrollMinY, SCROLLBAR_WIDTH, scrollBarTotalH, radius, tsl::Color(0x3, 0x3, 0x3, 0xD));
        r->drawRoundedRect(scrollBarX, scrollBarY, SCROLLBAR_WIDTH, scrollBarHeight, radius, tsl::Color(0x8, 0x8, 0x8, 0xF));
    }
    
    // 绘制进度条（下载/解压时）
    s32 innerX = x + 5, innerY = listY, innerW = w - 6, innerH = ITEM_HEIGHT;
    
    if (m_state == UpdateState::Downloading || m_state == UpdateState::Unzipping) {
        int percent = (m_state == UpdateState::Downloading) ? ult::downloadPercentage.load() : ult::unzipPercentage.load();
        s32 progressWidth = innerW * percent / 100;
        r->drawRect(innerX, innerY, progressWidth, innerH, tsl::Color(0x0, 0xB, 0xF, 0x7));
    }

    // 绘制底部按钮
    double progress = calcBlinkProgress();
    tsl::Color hlColor = calcBlinkColor(tsl::highlightColor1, tsl::highlightColor2, progress);
    
    // 绘制选中框
    r->drawBorderedRoundedRect(x, listY, w + 4, ITEM_HEIGHT, 5, 5, r->a(hlColor));
    
    // 绘制文字（根据状态显示不同内容）
    s32 keyFont = 23;
    s32 valFont = 20;
    
    std::string keyText, valText;
    tsl::Color keyColor = tsl::defaultTextColor;
    tsl::Color valColor = tsl::onTextColor;
    
    switch (m_state) {
        case UpdateState::Downloading:
            keyText = "正在下载";
            valText = std::to_string(ult::downloadPercentage.load()) + "%";
            valColor = tsl::Color(0xF, 0xF, 0x0, 0xF);  // 黄色
            break;
        case UpdateState::Unzipping:
            keyText = "正在解压";
            valText = std::to_string(ult::unzipPercentage.load()) + "%";
            valColor = tsl::Color(0xF, 0xF, 0x0, 0xF);  // 黄色
            break;
        case UpdateState::CancelUpdate:
            keyText = "已取消更新";
            valText = "按  更新";
            keyColor = tsl::Color(0xF, 0x5, 0x5, 0xF);  // 红色
            break;
        case UpdateState::UpdateError:
            keyText = "更新失败";
            valText = "按  更新";
            keyColor = tsl::Color(0xF, 0x5, 0x5, 0xF);  // 红色
            break;
        default:
            keyText = "更新插件";
            valText = "按  更新";
            break;
    }
    
    auto keyDim = r->getTextDimensions(keyText, false, keyFont);
    auto valDim = r->getTextDimensions(valText, false, valFont);
    
    s32 keyY = listY + (ITEM_HEIGHT + keyDim.second) / 2;
    s32 valY = listY + (ITEM_HEIGHT + valDim.second) / 2;
    
    r->drawString(keyText, false, x + 19, keyY, keyFont, r->a(keyColor));
    r->drawString(valText, false, x + w - 15 - valDim.first, valY, valFont, r->a(valColor));
}

