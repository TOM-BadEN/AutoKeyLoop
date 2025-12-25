#include "macro_list.hpp"
#include "macro_view.hpp"
#include <ultra.hpp>
#include "game.hpp"
#include <algorithm>
#include <strings.h>
#include "hiddata.hpp"
#include "refresh.hpp"
#include "macro_util.hpp"

namespace {
    bool s_isBack = false;  
}

// 脚本清单所有游戏列表类
MacroListGui::MacroListGui() 
 : m_macroDirs()
{
    // 正向进入消耗掉这个标志，但是什么都不做
    Refresh::RefrConsume(Refresh::MacroList);
    // 获取有脚本的所有游戏目录(目录名是titleId)
    auto dirs = MacroUtil::getGameDirs();
    m_macroDirs.reserve(dirs.size());
    for (const auto& dir : dirs) {
        m_macroDirs.push_back({dir.titleId, dir.macroCount, nullptr});
    }

}

tsl::elm::Element* MacroListGui::createUI() {
    auto frame = new tsl::elm::OverlayFrame("脚本列表", "管理录制的脚本");
    auto list = new tsl::elm::List();
    if (m_macroDirs.empty()) {
        auto noMacro = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
            s32 textFont = 40;
            s32 iconFont = 100;
            s32 gap = 40;
            auto textDim = r->getTextDimensions("未发现任何脚本", false, textFont);
            auto iconDim = r->getTextDimensions("", false, iconFont);
            s32 totalHeight = iconDim.second + gap + textDim.second;
            s32 blockTop = y + (h - totalHeight) / 2;
            s32 iconX = x + (w - iconDim.first) / 2;
            s32 iconY = blockTop + iconDim.second;
            s32 textX = x + (w - textDim.first) / 2;
            s32 textY = iconY + gap + textDim.second;
            r->drawString("", false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
            r->drawString("未发现任何脚本", false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
        });
    
        list->addItem(noMacro, 520);
        frame->setContent(list);
        return frame;
    }

    list->addItem(new tsl::elm::CategoryHeader(" 选择要查看的游戏"));
    for (auto& entry : m_macroDirs) {
        auto item = new tsl::elm::ListItem("", std::to_string(entry.macroCount));
        entry.item = item;
        item->setClickListener([this, entry](u64 keys) {
            if (keys & HidNpadButton_A) {
                u64 tid = strtoull(entry.dirName.c_str(), nullptr, 16);
                tsl::changeTo<MacroListGuiGame>(tid);
                return true;
            }
            return false;
        });
        list->addItem(item);
    }

    frame->setContent(list);
    return frame;
}

// 异步更新游戏名字
void MacroListGui::update() {

    // 如果是返回来的重新刷新
    if (s_isBack) {
        s_isBack = false;
        if (m_macroDirs.empty()) {
            tsl::goBack();
            return;
        }
    }

    if (m_nextIndex >= m_macroDirs.size()) return;
    auto& entry = m_macroDirs[m_nextIndex];
    if (!entry.item) {
        m_nextIndex = m_nextIndex + 1;
        return;
    }
    u64 tid = strtoull(entry.dirName.c_str(), nullptr, 16);
    char nameBuf[64]{};
    if (GameMonitor::getTitleIdGameName(tid, nameBuf)) entry.item->setText(nameBuf);
    else entry.item->setText(entry.dirName);
    m_nextIndex = m_nextIndex + 1;
}


// 脚本清单具体游戏的脚本列表类
MacroListGuiGame::MacroListGuiGame(u64 titleId)
 : m_titleId(titleId)
{
    // 如果这个界面是首次创建，则消耗刷新标志，避免引发崩溃
    Refresh::RefrConsume(Refresh::MacroGameList);
    GameMonitor::getTitleIdGameName(m_titleId, m_gameName);
    // 获取所有宏（已排序：有快捷键的在前）
    m_macro = MacroUtil::getMacroList(m_titleId);
}

tsl::elm::Element* MacroListGuiGame::createUI() {
    auto frame = new tsl::elm::OverlayFrame(m_gameName, "当前游戏的所有脚本");
    auto list = new tsl::elm::List();
    if (m_macro.empty()) {
        auto noMacro = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
            s32 textFont = 40;
            s32 iconFont = 100;
            s32 gap = 40;
            auto textDim = r->getTextDimensions("未发现任何脚本", false, textFont);
            auto iconDim = r->getTextDimensions("", false, iconFont);
            s32 totalHeight = iconDim.second + gap + textDim.second;
            s32 blockTop = y + (h - totalHeight) / 2;
            s32 iconX = x + (w - iconDim.first) / 2;
            s32 iconY = blockTop + iconDim.second;
            s32 textX = x + (w - textDim.first) / 2;
            s32 textY = iconY + gap + textDim.second;
            r->drawString("", false, iconX, iconY, iconFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
            r->drawString("未发现任何脚本", false, textX, textY, textFont, tsl::Color(0xF, 0xF, 0xF, 0xF));
        });
    
        list->addItem(noMacro, 520);
        frame->setContent(list);
        return frame;
    }

    list->addItem(new tsl::elm::CategoryHeader(" 选择要查看的脚本"));
    for (const auto& entry : m_macro) {
        std::string displayName = MacroUtil::getDisplayName(entry.path);
        auto item = new tsl::elm::ListItem(displayName, HidHelper::getCombinedIcons(entry.hotkey));
        item->setClickListener([this, entry](u64 keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<MacroViewGui>(entry.path.c_str(), m_gameName);
                return true;
            }   
            return false;
        });
        list->addItem(item);   
    }

    frame->setContent(list);
    return frame;

}

void MacroListGuiGame::update() {
    if (Refresh::RefrConsume(Refresh::MacroGameList)) {
        u64 tid = m_titleId;
        tsl::swapTo<MacroListGuiGame>(SwapDepth(1), tid);
        return;
    }
}

bool MacroListGuiGame::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysDown & HidNpadButton_B) {
        u64 true_Tid = GameMonitor::getCurrentTitleId();
        if (Refresh::RefrConsume(Refresh::MacroList) && true_Tid == 0) {
            s_isBack = true;
            tsl::swapTo<MacroListGui>(SwapDepth(2));
            return true;
        } else {
            tsl::goBack();
            return true;
        }
    }

    return false;
}






