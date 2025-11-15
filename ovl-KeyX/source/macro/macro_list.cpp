#include "macro_list.hpp"
#include "macro_view.hpp"
#include <ultra.hpp>
#include "game.hpp"
#include <algorithm>
#include <strings.h>
#include "ini_helper.hpp"
#include "hiddata.hpp"
#include "refresh.hpp"

namespace {
    const char* MACROS_DIR = "/config/KeyX/macros";
}

// 脚本清单所有游戏列表类
MacroListGui::MacroListGui() 
 : m_macroDirs()
{
    // 获取有脚本的所有游戏目录(目录名是titleId)
    auto dirs = ult::getSubdirectories(MACROS_DIR);
    m_macroDirs.reserve(dirs.size());
    for (const auto& dir : dirs) {
        MacroDirEntry entry{};
        entry.dirName = dir;
        m_macroDirs.push_back(entry);
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
        auto item = new tsl::elm::ListItem("");
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
    // 获取游戏名
    GameMonitor::getTitleIdGameName(m_titleId, m_gameName);
    // 获取所有宏文件
    char gameDirPath[64]{};
    sprintf(gameDirPath, "sdmc:/config/KeyX/macros/%016lX/*.macro", m_titleId);
    std::vector<std::string> allMacroFiles = ult::getFilesListByWildcards(gameDirPath);
    // 按名称排序
    std::sort(allMacroFiles.begin(), allMacroFiles.end(),[](const std::string& a, const std::string& b) { return strcasecmp(a.c_str(), b.c_str()) < 0; });
    // 获取已经设置了快捷键的宏文件
    char gameCfgPath[64];
    sprintf(gameCfgPath, "sdmc:/config/KeyX/GameConfig/%016lX.ini", m_titleId);
    Macro entry;
    int macroCount = IniHelper::getInt("MACRO", "macroCount", 0, gameCfgPath);
    for (int idx = 1; idx <= macroCount; ++idx) {
        std::string path  = "macro_path_"  + std::to_string(idx);
        std::string combo = "macro_combo_" + std::to_string(idx);
        std::string macroPath = IniHelper::getString("MACRO", path, "", gameCfgPath);
        if (macroPath.empty()) continue;
        u64 Hotkey = static_cast<u64>(IniHelper::getInt("MACRO", combo, 0, gameCfgPath));
        if (Hotkey == 0) continue;
        entry.macroPath = macroPath;
        entry.Hotkey = Hotkey;
        m_macro.push_back(entry);
    }
    // 按名称排序
    std::sort(m_macro.begin(), m_macro.end(),[](const Macro& lhs, const Macro& rhs) {
        std::string nameL = ult::getFileName(lhs.macroPath);
        std::string nameR = ult::getFileName(rhs.macroPath);
        return strcasecmp(nameL.c_str(), nameR.c_str()) < 0;
    });
    // 去重和拼接成完整的宏文件列表
    std::unordered_set<std::string> boundPaths;
    boundPaths.reserve(m_macro.size());
    for (const auto& entry : m_macro) boundPaths.insert(entry.macroPath);
    for (const auto& filePath : allMacroFiles) {
        if (boundPaths.count(filePath)) continue;
        Macro extra{};
        extra.macroPath = filePath;
        extra.Hotkey = 0;                             
        m_macro.push_back(std::move(extra));
    }
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
        std::string fileName = ult::getFileName(entry.macroPath);
        auto dot = fileName.rfind('.');
        if (dot != std::string::npos) fileName = fileName.substr(0, dot); 
        auto item = new tsl::elm::ListItem(fileName, HidHelper::getCombinedIcons(entry.Hotkey));
        item->setClickListener([this, entry](u64 keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<MacroViewGui>(entry.macroPath.c_str(), m_gameName);
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
        tsl::goBack(); 
        tsl::changeTo<MacroListGuiGame>(tid);    // 删除当前界面重新创建
    }
}





