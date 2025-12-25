#include "store_ui.hpp"
#include <functional>
#include <atomic>
#include <chrono>
#include <cmath>
#include <ultra.hpp>
#include "game.hpp"
#include "Tthread.hpp"
#include "info_edit.hpp"
#include "language.hpp"
#include "qrcodegen.hpp"
#include "memory.hpp"

using qrcodegen::QrCode;
using qrcodegen::QrSegment;

namespace {
    GameListResult s_gameList;
    MacroListResult s_macroList;
    StoreMacroEntry s_selectedMacro;
    bool s_downloadSuccess = false;
    
    constexpr const char* s_refreshIcon[] = {"", "", "", "", "", "", "", ""};
    constexpr const char* EDIT_URL = "https://macro.dokiss.cn/edit_prop.php?tid=%s&file=%s&lang=%s";
    constexpr const char* langParam[] = {"zh", "zhh", "en"};
    constexpr const char* STORE_URL = "https://macro.dokiss.cn/store.php?tid=%s&lang=%s";
    
    // 使用说明布局常量（参考 updater_ui.cpp）
    constexpr s32 DESC_FONT_SIZE = 18;
    constexpr s32 DESC_LINE_HEIGHT = 26;
    constexpr s32 SCROLLBAR_WIDTH = 3;
    constexpr s32 SCROLLBAR_GAP = 8;
    constexpr s32 ITEM_HEIGHT = 70;
    
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
        
        // 按 \n 分割（注意这里是单字符 \n）
        std::string remaining = desc;
        size_t pos = 0;
        while ((pos = remaining.find('\n')) != std::string::npos) {
            std::string item = remaining.substr(0, pos);
            remaining = remaining.substr(pos + 1);
            
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
}


// ==================== StoreGetDataGui ====================


StoreGetDataGui::StoreGetDataGui(u64 tid) : m_tid(tid) {

    s_gameList.games.clear();
    s_macroList.macros.clear();
    s_selectedMacro = {};
    
    if (tid == 0) {
        Thd::start([]{ s_gameList = StoreData().getGameList(); });
    } else {
        Thd::start([tid]{ getMacroListForTid(tid); });
        GameMonitor::getTitleIdGameName(m_tid, gameName);
    }
}

StoreGetDataGui::~StoreGetDataGui() {
    Thd::stop();
}

tsl::elm::Element* StoreGetDataGui::createUI() {
    std::string subtitle = (m_tid == 0) ? "商店脚本靠大家，欢迎投稿" : gameName;
    auto frame = new tsl::elm::OverlayFrame("脚本商店", subtitle);
    auto list = new tsl::elm::List();
    
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        s32 iconFont = 150;
        s32 textFont = 32;
        s32 gap = 55;
        s32 lineGap = 40;
        
        std::string icon;
        std::string text;
        std::string text2;
        
        switch (m_state) {
            case LoadState::Loading:
                icon = s_refreshIcon[m_frameIndex];
                text = "正在检查数据";
                break;
            case LoadState::NetworkError:
                icon = "";
                text = s_gameList.error.empty() ? s_macroList.error : s_gameList.error;
                text2 = "请检查时间同步";
                break;
            case LoadState::GameListEmpty:
            case LoadState::MacroListEmpty:
                icon = "";
                text = "未发现可用的脚本";
                text2 = "欢迎向我投稿作品";
                break;
            case LoadState::Success:
                return;
        }
        
        auto iconDim = r->getTextDimensions(icon, false, iconFont);
        auto textDim = r->getTextDimensions(text, false, textFont);
        auto text2Dim = r->getTextDimensions(text2, false, textFont);
        
        s32 totalHeight = iconDim.second + gap + textDim.second;
        if (!text2.empty()) totalHeight += lineGap + text2Dim.second;
        s32 blockTop = y + (h - totalHeight) / 2;
        
        s32 iconX = x + (w - iconDim.first) / 2;
        s32 iconY = blockTop + iconDim.second;
        
        s32 textX = x + (w - textDim.first) / 2;
        s32 textY = iconY + gap + textDim.second;
        
        r->drawString(icon, false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
        r->drawString(text, false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
        
        if (!text2.empty()) {
            s32 text2X = x + (w - text2Dim.first) / 2;
            s32 text2Y = textY + lineGap + text2Dim.second;
            r->drawString(text2, false, text2X, text2Y, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
        }
    });
    
    list->addItem(drawer, 520);
    frame->setContent(list);
    return frame;
}

void StoreGetDataGui::update() {
    if (m_state == LoadState::Loading) {
        m_frameCounter++;
        if (m_frameCounter >= 12) {
            m_frameCounter = 0;
            m_frameIndex = (m_frameIndex + 1) % 8;
        }
        
        if (Thd::isDone() && m_tid == 0) {
            Thd::stop();
            if (!s_gameList.success) m_state = LoadState::NetworkError;
            else if (s_gameList.games.empty()) m_state = LoadState::GameListEmpty;
            else {
                m_state = LoadState::Success;
                tsl::swapTo<StoreGameListGui>();
            }
        }
        else if (Thd::isDone() && m_tid != 0) {
            Thd::stop();
            if (!s_gameList.success) m_state = LoadState::NetworkError;
            else if (!s_macroList.success) m_state = LoadState::NetworkError;
            else if (s_macroList.macros.empty()) m_state = LoadState::MacroListEmpty;
            else {
                m_state = LoadState::Success;
                tsl::swapTo<StoreMacroListGui>(m_tid, gameName, true);
            }
        }
    }
}

bool StoreGetDataGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_B) {
        ult::abortDownload = true;
        Thd::stop();
        s_gameList.games.clear();
        s_gameList.games.shrink_to_fit();
        s_macroList.macros.clear();
        s_macroList.macros.shrink_to_fit();
        s_selectedMacro = {};
        tsl::goBack();
        return true;
    }
    return false;
}

void StoreGetDataGui::getMacroListForTid(u64 tid) {
    s_gameList = StoreData().getGameList();
    if (!s_gameList.success) return;
    
    bool found = false;
    for (const auto& g : s_gameList.games) {
        if (g.id == tid) { found = true; break; }
    }
    if (!found) {
        s_macroList = MacroListResult{};
        s_macroList.success = true;
        return;
    }
    
    char tidStr[17];
    snprintf(tidStr, 17, "%016lX", tid);
    s_macroList = StoreData().getMacroList(tidStr);
}
// ==================== StoreGameListGui ====================

StoreGameListGui::StoreGameListGui() {
    m_entries.reserve(s_gameList.games.size());
    for (const auto& game : s_gameList.games) {
        m_entries.push_back({game.id, nullptr});
    }
}

StoreGameListGui::~StoreGameListGui() {
    Thd::stop();
}

tsl::elm::Element* StoreGameListGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("脚本商店", "已安装的游戏列表");
    
    auto list = new tsl::elm::List();
    list->addItem(new tsl::elm::CategoryHeader(" 选择对应游戏"));
    
    for (size_t i = 0; i < s_gameList.games.size(); i++) {
        char tidStr[17];
        snprintf(tidStr, sizeof(tidStr), "%016lX", s_gameList.games[i].id);
        auto item = new tsl::elm::ListItem(tidStr, std::to_string(s_gameList.games[i].count));
        item->setClickListener([this, i](u64 keys) {
            if (keys & HidNpadButton_A) {
                if (m_loadingIndex >= 0) return false;
                m_loadingIndex = i;
                m_entries[i].item->setValueColor(tsl::onTextColor);  // 恢复青色
                u64 tid = m_entries[i].tid;
                Thd::start([tid] {
                    char tidStr[17];
                    snprintf(tidStr, 17, "%016lX", tid);
                    s_macroList = StoreData().getMacroList(tidStr);
                });
                return true;
            }
            return false;
        });
        m_entries[i].item = item;
        list->addItem(item);
    }
    
    frame->setContent(list);
    return frame;
}

void StoreGameListGui::update() {
    // 更新游戏名
    if (m_nextIndex < m_entries.size()) {
        auto& entry = m_entries[m_nextIndex];
        if (entry.item) {
            char nameBuf[64]{};
            if (GameMonitor::getTitleIdGameName(entry.tid, nameBuf)) {
                entry.item->setText(nameBuf);
            }
        }
        m_nextIndex++;
    }
    
    // 加载动画和结果处理
    if (m_loadingIndex >= 0) {
        m_frameCounter++;
        if (m_frameCounter >= 12) {
            m_frameCounter = 0;
            m_frameIndex = (m_frameIndex + 1) % 8;
        }
        m_entries[m_loadingIndex].item->setValue(s_refreshIcon[m_frameIndex]);
        
        if (Thd::isDone()) {
            Thd::stop();
            int idx = m_loadingIndex;
            m_loadingIndex = -1;
            if (s_macroList.success) {
                m_entries[idx].item->setValue(std::to_string(s_gameList.games[idx].count));
                std::string gameName = m_entries[idx].item->getText();
                tsl::changeTo<StoreMacroListGui>(m_entries[idx].tid, gameName);
            }
            else {
                m_entries[idx].item->setValue(s_macroList.error);
                m_entries[idx].item->setValueColor(tsl::warningTextColor);
            }
        }
    }
}

bool StoreGameListGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_B) {
        ult::abortDownload = true;
        Thd::stop();
        s_gameList.games.clear();
        s_gameList.games.shrink_to_fit();
        s_macroList.macros.clear();
        s_macroList.macros.shrink_to_fit();
        s_selectedMacro = {};
        tsl::goBack();
        return true;
    }
    // 加载期间屏蔽其他按键，非加载期间交给框架
    return (m_loadingIndex >= 0);
}


// ==================== StoreMacroListGui ====================

StoreMacroListGui::StoreMacroListGui(u64 tid, const std::string& gameName, bool fromGame)
    : m_tid(tid), m_gameName(gameName), m_fromGame(fromGame) {
        
    MemMonitor::log("获取完成，跳转到宏列表");
}

StoreMacroListGui::~StoreMacroListGui() {
    Thd::stop();
}

tsl::elm::Element* StoreMacroListGui::createUI() {
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    frame->setHeader(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("脚本商店", false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString(m_gameName, false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
        renderer->drawString("  网页商店", false, 270, 693, 23, renderer->a(tsl::style::color::ColorText));
    }));
    
    auto list = new tsl::elm::List();
    list->addItem(new tsl::elm::CategoryHeader(" 点击查看脚本详情"));
    
    for (size_t i = 0; i < s_macroList.macros.size(); i++) {
        const auto& macro = s_macroList.macros[i];
        auto item = new tsl::elm::ListItem(macro.name, macro.author);
        item->setClickListener([this, i](u64 keys) {
            if (keys & HidNpadButton_A) {
                s_selectedMacro = s_macroList.macros[i];
                tsl::changeTo<StoreMacroViewGui>(m_tid, m_gameName);
                return true;
            }
            return false;
        });
        list->addItem(item);
    }
    
    frame->setContent(list);
    return frame;
}

bool StoreMacroListGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_Right) {
        tsl::changeTo<WebStoreGui>(m_tid, m_gameName);
        return true;
    }
    if (keysDown & HidNpadButton_B) {
        if (m_fromGame) {
            s_gameList.games.clear();
            s_gameList.games.shrink_to_fit();
            s_macroList.macros.clear();
            s_macroList.macros.shrink_to_fit();
            s_selectedMacro = {};
        }
        MemMonitor::log("退出商店");
        tsl::goBack();
        return true;
    }
    return false;
}


// ==================== StoreMacroViewGui ====================

StoreMacroViewGui::StoreMacroViewGui(u64 tid, const std::string& gameName)
    : m_tid(tid), m_gameName(gameName) {
        
    MemMonitor::log("跳转到宏详情");
    // 计算下载路径
    char tidStr[17];
    snprintf(tidStr, sizeof(tidStr), "%016lX", tid);
    std::string dirPath = "sdmc:/config/KeyX/macros/" + std::string(tidStr) + "/";
    std::string fileName = s_selectedMacro.file;
    std::string baseName = fileName.substr(0, fileName.size() - 6);
    m_localPath = dirPath + fileName;
    m_isInstalled = ult::isFile(m_localPath);
}

StoreMacroViewGui::~StoreMacroViewGui() {
    Thd::stop();
}

tsl::elm::Element* StoreMacroViewGui::createUI() {
    auto frame = new tsl::elm::HeaderOverlayFrame(97);
    frame->setHeader(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("脚本商店", false, 20, 50+2, 32, renderer->a(tsl::defaultOverlayColor));
        renderer->drawString(m_gameName, false, 20, 50+23, 15, renderer->a(tsl::bannerVersionTextColor));
        if (m_state == MacroViewState::Ready || m_state == MacroViewState::Error || m_state == MacroViewState::Cancelled) {
            renderer->drawString("  编辑", false, 270, 693, 23, renderer->a(tsl::style::color::ColorText));
        }
    }));
    auto list = new tsl::elm::List();
    auto textArea = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        drawContent(r, x, y, w, h);
    });
    list->addItem(textArea, 520);
    frame->setContent(list);
    return frame;
}

void StoreMacroViewGui::update() {
    if (m_state == MacroViewState::Downloading && Thd::isDone()) {
        Thd::stop();
        m_state = s_downloadSuccess ? MacroViewState::Success : MacroViewState::Error;
        MemMonitor::log("下载完成");
    }
}

bool StoreMacroViewGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    // 上下键滚动使用说明（参考 updater_ui.cpp）
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
    
    if (m_state == MacroViewState::Ready || m_state == MacroViewState::Error || m_state == MacroViewState::Cancelled) {
        if (keysDown & HidNpadButton_Plus) {
            m_state = MacroViewState::Downloading;
            MemMonitor::log("开始下载宏");
            char tidStr[17];
            snprintf(tidStr, sizeof(tidStr), "%016lX", m_tid);
            std::string gameId = tidStr;
            std::string fileName = s_selectedMacro.file;
            s_selectedMacro.file = m_localPath.substr(m_localPath.find_last_of('/') + 1);
            std::string localPath = m_localPath;
            Thd::start([gameId, fileName, localPath] {
                s_downloadSuccess = StoreData().downloadMacro(gameId, fileName, localPath);
                if (s_downloadSuccess) StoreData::saveMacroMetadataInfo(gameId, s_selectedMacro);
            });
            return true;
        }
        if (keysDown & HidNpadButton_Right) {
            char tidStr[17];
            snprintf(tidStr, sizeof(tidStr), "%016lX", m_tid);
            int langIndex = LanguageManager::getZhcnOrZhtwOrEnIndex();
            char editUrl[128];
            snprintf(editUrl, sizeof(editUrl), EDIT_URL, tidStr, s_selectedMacro.file.c_str(), langParam[langIndex]);
            tsl::changeTo<MacroInfoEditGui>(editUrl, s_selectedMacro.name, s_selectedMacro.author);
            return true;
        }
    }
    if (m_state == MacroViewState::Downloading) {
        if (keysDown & HidNpadButton_B) {
            ult::abortDownload = true;
            Thd::stop();
            m_state = MacroViewState::Cancelled;
        }
        return true;
    }
    return false;
}

void StoreMacroViewGui::drawContent(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    constexpr s32 ITEM_HEIGHT = 72;
    s32 textX = x + 19;
    s32 currentY = y + 35;
    
    // 脚本名称 + 作者（青色）
    s32 nameFontSize = 28;
    s32 byFontSize = 20;
    std::string byText = " by " + s_selectedMacro.author;
    auto nameDim = r->getTextDimensions(s_selectedMacro.name, false, nameFontSize);
    auto byDim = r->getTextDimensions(byText, false, byFontSize);
    while (nameDim.first + byDim.first > w - 19 - 15) {
        nameFontSize = nameFontSize * 9 / 10;
        byFontSize = byFontSize * 9 / 10;
        nameDim = r->getTextDimensions(s_selectedMacro.name, false, nameFontSize);
        byDim = r->getTextDimensions(byText, false, byFontSize);
    }
    r->drawString(s_selectedMacro.name, false, textX, currentY, nameFontSize, r->a(tsl::onTextColor));
    r->drawString(byText, false, textX + nameDim.first, currentY, byFontSize, r->a(tsl::onTextColor));
    currentY += 50;
    
    // 通用绘制参数
    s32 maxWidth = w - 19 - 15;
    s32 fontSize = 18;
    s32 lineHeight = 26;
    
    // 下载次数
    r->drawString("下载次数:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    r->drawString(" • " + std::to_string(s_selectedMacro.downloads), false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight + 20;
    
    // ========== 使用说明（可滚动区域）==========
    r->drawString("使用说明:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    
    // 计算区域边界（参考 updater_ui.cpp）
    s32 listYPos = y + h - 10 - ITEM_HEIGHT;
    s32 textMinY = currentY;
    s32 descMaxY = listYPos - 22;
    s32 visibleHeight = descMaxY - textMinY;
    s32 scrollMinY = textMinY - DESC_LINE_HEIGHT + 5;
    
    // 预处理使用说明
    std::string prefix = " • ";
    auto [prefixW, prefixH] = r->getTextDimensions(prefix, false, DESC_FONT_SIZE);
    s32 descMaxWidth = maxWidth - SCROLLBAR_WIDTH - SCROLLBAR_GAP;
    
    // 懒加载：只在首帧计算
    if (!m_descReady) {
        auto lines = preprocessDesc(r, s_selectedMacro.desc, descMaxWidth, prefixW, m_descTotalHeight);
        m_descLines.reserve(lines.size());
        for (const auto& line : lines) {
            m_descLines.emplace_back(line.text, line.hasPrefix);
        }
        m_descReady = true;
    }
    
    // 更新滚动状态
    m_maxScrollOffset = (m_descTotalHeight > visibleHeight) ? (m_descTotalHeight - visibleHeight) : 0;
    if (m_scrollOffset > m_maxScrollOffset) m_scrollOffset = m_maxScrollOffset;
    if (m_scrollOffset < 0) m_scrollOffset = 0;
    
    // 绘制使用说明
    s32 maxLines = (visibleHeight + DESC_LINE_HEIGHT - 1) / DESC_LINE_HEIGHT;
    s32 contentY = 0, drawY = textMinY, drawnLines = 0;
    
    for (const auto& [text, hasPrefix] : m_descLines) {
        if (drawnLines >= maxLines) break;
        if (contentY >= m_scrollOffset) {
            if (hasPrefix) {
                r->drawString(prefix + text, false, textX, drawY, DESC_FONT_SIZE, r->a(tsl::style::color::ColorDescription));
            } else {
                r->drawString(text, false, textX + prefixW, drawY, DESC_FONT_SIZE, r->a(tsl::style::color::ColorDescription));
            }
            drawY += DESC_LINE_HEIGHT;
            drawnLines++;
        }
        contentY += DESC_LINE_HEIGHT;
    }
    
    // 绘制滚动条
    if (m_maxScrollOffset > 0) {
        s32 scrollBarX = x + w - SCROLLBAR_WIDTH;
        s32 scrollBarTotalH = descMaxY - scrollMinY;
        s32 scrollBarHeight = scrollBarTotalH * visibleHeight / m_descTotalHeight;
        if (scrollBarHeight < 20) scrollBarHeight = 20;
        s32 scrollBarY = scrollMinY + (scrollBarTotalH - scrollBarHeight) * m_scrollOffset / m_maxScrollOffset;
        s32 radius = SCROLLBAR_WIDTH / 2;
        
        r->drawRoundedRect(scrollBarX, scrollMinY, SCROLLBAR_WIDTH, scrollBarTotalH, radius, tsl::Color(0x3, 0x3, 0x3, 0xD));
        r->drawRoundedRect(scrollBarX, scrollBarY, SCROLLBAR_WIDTH, scrollBarHeight, radius, tsl::Color(0x8, 0x8, 0x8, 0xF));
    }
    
    // 绘制底部按钮
    
    // 绘制选中框
    double progress = (std::sin(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() / 300.0) + 1.0) / 2.0;
    tsl::Color hlColor = {
        static_cast<u8>(tsl::highlightColor1.r + (tsl::highlightColor2.r - tsl::highlightColor1.r) * progress),
        static_cast<u8>(tsl::highlightColor1.g + (tsl::highlightColor2.g - tsl::highlightColor1.g) * progress),
        static_cast<u8>(tsl::highlightColor1.b + (tsl::highlightColor2.b - tsl::highlightColor1.b) * progress),
        tsl::highlightColor1.a
    };
    r->drawBorderedRoundedRect(x, listYPos, w + 4, ITEM_HEIGHT, 5, 5, r->a(hlColor));
    
    // 绘制进度条（下载中）
    if (m_state == MacroViewState::Downloading) {
        s32 border = 5;
        s32 innerX = x + border;
        s32 innerW = w + 4 - border * 2;
        int percent = ult::downloadPercentage.load();
        s32 progressWidth = innerW * percent / 100;
        r->drawRect(innerX, listYPos, progressWidth, ITEM_HEIGHT, tsl::Color(0x0, 0xB, 0xF, 0x7));
    }
    
    // 绘制按钮文字
    s32 keyFont = 23;
    s32 valFont = 20;
    std::string keyText, valText;
    tsl::Color keyColor = tsl::defaultTextColor;
    tsl::Color valColor = tsl::onTextColor;
    
    switch (m_state) {
        case MacroViewState::Ready:
            keyText = m_isInstalled ? "已安装" : "下载脚本";
            valText = m_isInstalled ? "按  更新" : "按  下载";
            keyColor = m_isInstalled ? tsl::Color(0xF, 0xF, 0x0, 0xF) : tsl::defaultTextColor;
            break;
        case MacroViewState::Downloading:
            keyText = "正在下载";
            keyColor = tsl::defaultTextColor;
            valText = std::to_string(ult::downloadPercentage.load()) + "%";
            valColor = tsl::Color(0xF, 0xF, 0x0, 0xF);  // 黄色
            break;
        case MacroViewState::Success:
            keyText = "下载成功";
            valText = "按  返回";
            keyColor = tsl::defaultTextColor;
            break;
        case MacroViewState::Cancelled:
            keyText = "取消成功";
            valText = "按  下载";
            keyColor = tsl::Color(0xF, 0x5, 0x5, 0xF);  // 红色
            break;
        case MacroViewState::Error:
            keyText = "下载失败";
            valText = "按  下载";
            keyColor = tsl::Color(0xF, 0x5, 0x5, 0xF);  // 红色
            break;
    }
    
    auto keyDim = r->getTextDimensions(keyText, false, keyFont);
    auto valDim = r->getTextDimensions(valText, false, valFont);
    
    s32 keyY = listYPos + (ITEM_HEIGHT + keyDim.second) / 2;
    s32 valY = listYPos + (ITEM_HEIGHT + valDim.second) / 2;
    
    r->drawString(keyText, false, x + 19, keyY, keyFont, r->a(keyColor));
    r->drawString(valText, false, x + w - 15 - valDim.first, valY, valFont, r->a(valColor));
}


// ==================== WebStoreGui ====================

WebStoreGui::WebStoreGui(u64 tid, const std::string& gameName)
 : m_tid(tid)
 , m_gameName(gameName)
 {
    // 生成二维码 URL
    char tidStr[17];
    snprintf(tidStr, sizeof(tidStr), "%016lX", m_tid);
    int langIndex = LanguageManager::getZhcnOrZhtwOrEnIndex();
    char storeUrl[128];
    snprintf(storeUrl, sizeof(storeUrl), STORE_URL, tidStr, langParam[langIndex]);
    
    // 生成二维码
    QrCode qr = QrCode::encodeSegments(QrSegment::makeSegments(storeUrl), QrCode::Ecc::LOW, 5, 5);
    m_qrSize = qr.getSize();
    m_qrModules.resize(m_qrSize);
    for (int qy = 0; qy < m_qrSize; qy++) {
        m_qrModules[qy].resize(m_qrSize);
        for (int qx = 0; qx < m_qrSize; qx++) {
            m_qrModules[qy][qx] = qr.getModule(qx, qy);
        }
    }
}

tsl::elm::Element* WebStoreGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("网页商店", "网页已适配手机和电脑端");
    
    auto list = new tsl::elm::List();
    auto drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        drawContent(r, x, y, w, h);
    });
    list->addItem(drawer, 520);
    
    frame->setContent(list);
    return frame;
}

void WebStoreGui::drawContent(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
    s32 textX = x + 19;
    s32 currentY = y + 35;
    s32 fontSize = 18;
    s32 lineHeight = 26;
    
    // 大标题：游戏名（自动缩小）
    s32 nameFontSize = 28;
    auto nameDim = r->getTextDimensions(m_gameName, false, nameFontSize);
    while (nameDim.first > w - 19 - 15) {
        nameFontSize = nameFontSize * 9 / 10;
        nameDim = r->getTextDimensions(m_gameName, false, nameFontSize);
    }
    r->drawString(m_gameName, false, textX, currentY, nameFontSize, r->a(tsl::onTextColor));
    currentY += 45;
    
    // 功能介绍
    r->drawString("功能介绍:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    r->drawString(" • 扫描二维码，可直接访问网页端", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight;
    r->drawString(" • 网页商店功能正在逐步完善", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight + 20;
    
    // 编辑方法
    r->drawString("特别感谢:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    r->drawString(" • 忘忧 (dokiss.cn)", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight;
    r->drawString(" • 感谢提供服务器与制作网页", false, textX, currentY, fontSize, r->a(tsl::style::color::ColorDescription));
    currentY += lineHeight + 20;
    
    r->drawString("祝您游戏愉快！", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 37;
    
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

bool WebStoreGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_Left) {
        tsl::goBack();
        return true;
    }
    return false;
}