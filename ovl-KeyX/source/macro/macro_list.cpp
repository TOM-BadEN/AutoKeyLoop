#include "macro_list.hpp"
#include <ultra.hpp>
#include "game.hpp"


namespace {
    const char* MACROS_DIR = "/config/KeyX/macros";
}

MacroListGui::MacroListGui() 
 : m_macroDirs()
{
    if (ult::isDirectory(MACROS_DIR)) {
        auto dirs = ult::getSubdirectories(MACROS_DIR);
        m_macroDirs.reserve(dirs.size());
        for (const auto& dir : dirs) {
            MacroDirEntry entry{};
            entry.dirName = dir;
            m_macroDirs.push_back(entry);
        }
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
        auto item = new tsl::elm::ListItem(entry.dirName);
        entry.item = item;
        item->setClickListener([this](u64 keys) {
            if (keys & HidNpadButton_A) {
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
    if (m_nextIndex >= m_macroDirs.size()) return;
    auto& entry = m_macroDirs[m_nextIndex];
    if (!entry.item) {
        m_nextIndex = m_nextIndex + 1;
        return;
    }
    u64 tid = strtoull(entry.dirName.c_str(), nullptr, 16);
    char nameBuf[64]{};
    if (GameMonitor::getTitleIdGameName(tid, nameBuf)) entry.item->setText(nameBuf);
    m_nextIndex = m_nextIndex + 1;
}

