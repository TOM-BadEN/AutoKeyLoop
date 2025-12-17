#include "store_ui.hpp"
#include <functional>
#include <atomic>
#include <chrono>
#include <cmath>
#include <ultra.hpp>
#include "game.hpp"
#include "Tthread.hpp"

namespace {
    GameListResult s_gameList;
    MacroListResult s_macroList;
    StoreMacroEntry s_selectedMacro;
    bool s_downloadSuccess = false;
    
    constexpr const char* s_refreshIcon[] = {"", "", "", "", "", "", "", ""};
}


// ==================== StoreGetDataGui ====================


StoreGetDataGui::StoreGetDataGui(u64 tid) : m_tid(tid) {
    if (tid == 0) {
        Thd::start([]{ s_gameList = StoreData().getGameList(); });
    } else {
        Thd::start([tid]{ getMacroListForTid(tid); });
    }
}

StoreGetDataGui::~StoreGetDataGui() {
    Thd::stop();
}

tsl::elm::Element* StoreGetDataGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("脚本商店", "商店脚本靠大家，欢迎投稿");
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
                char nameBuf[64]{};
                GameMonitor::getTitleIdGameName(m_tid, nameBuf);
                tsl::swapTo<StoreMacroListGui>(m_tid, nameBuf);
            }
        }
    }
}

bool StoreGetDataGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_B) {
        ult::abortDownload = true;
        Thd::stop();
        s_gameList = {};
        s_macroList = {};
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
    if (m_loadingIndex >= 0) return true;
    if (keysDown & HidNpadButton_B) {
        ult::abortDownload = true;
        Thd::stop();
        s_gameList = {};
        s_macroList = {};
        tsl::goBack();
        return true;
    }
    return false;
}


// ==================== StoreMacroListGui ====================

StoreMacroListGui::StoreMacroListGui(u64 tid, const std::string& gameName)
    : m_tid(tid), m_gameName(gameName) {
}

StoreMacroListGui::~StoreMacroListGui() {
    Thd::stop();
}

tsl::elm::Element* StoreMacroListGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("脚本商店", m_gameName);
    
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

// ==================== StoreMacroViewGui ====================

StoreMacroViewGui::StoreMacroViewGui(u64 tid, const std::string& gameName)
    : m_tid(tid), m_gameName(gameName) {
    // 计算下载路径
    char tidStr[17];
    snprintf(tidStr, sizeof(tidStr), "%016lX", tid);
    std::string dirPath = "sdmc:/config/KeyX/Macros/" + std::string(tidStr) + "/";
    std::string fileName = s_selectedMacro.file;
    std::string baseName = fileName.substr(0, fileName.size() - 6);
    m_localPath = dirPath + fileName;
    int suffix = 1;
    while (ult::isFile(m_localPath)) {
        m_localPath = dirPath + baseName + "-" + std::to_string(suffix++) + ".macro";
    }
}

StoreMacroViewGui::~StoreMacroViewGui() {
    Thd::stop();
}

tsl::elm::Element* StoreMacroViewGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("脚本商店", m_gameName);
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
    }
}

bool StoreMacroViewGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (m_state == MacroViewState::Ready || m_state == MacroViewState::Error || m_state == MacroViewState::Cancelled) {
        if (keysDown & HidNpadButton_Plus) {
            m_state = MacroViewState::Downloading;
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
    
    // 下载路径
    r->drawString("下载路径:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    drawWrappedText(m_localPath, " • ");
    currentY += 20;
    
    // 使用说明
    r->drawString("使用说明:", false, textX, currentY, 20, r->a(tsl::defaultTextColor));
    currentY += 28;
    
    // 按 \n 分割描述文本
    std::string desc = s_selectedMacro.desc;
    size_t pos = 0;
    while ((pos = desc.find('\n')) != std::string::npos) {
        drawWrappedText(desc.substr(0, pos), " • ");
        desc = desc.substr(pos + 1);
    }
    if (!desc.empty()) {
        drawWrappedText(desc, " • ");
    }
    
    // 绘制底部按钮
    s32 listYPos = y + h - 10 - ITEM_HEIGHT;
    
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
            keyText = "下载脚本";
            valText = "按  下载";
            keyColor = tsl::defaultTextColor;
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
