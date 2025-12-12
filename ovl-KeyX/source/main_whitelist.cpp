#include "main_whitelist.hpp"
#include "game.hpp"
#include "ini_helper.hpp"
#include "ipc.hpp"

namespace {
    constexpr const char* WHITE_INI_PATH = "sdmc:/config/KeyX/white.ini";
}

SettingWhitelist::SettingWhitelist() {
    auto tids = GameMonitor::getInstalledAppIds();
    m_apps.reserve(tids.size());
    for (u64 tid : tids) {
        m_apps.push_back({tid, nullptr});
    }
}

tsl::elm::Element* SettingWhitelist::createUI() {
    auto frame = new tsl::elm::OverlayFrame("设置白名单", "允许按键助手在非游戏应用生效");
    auto list = new tsl::elm::List();

    list->addItem(new tsl::elm::CategoryHeader(" 设置后立即生效"));

    for (auto& entry : m_apps) {
        char tidStr[17];
        snprintf(tidStr, sizeof(tidStr), "%016lX", entry.tid);
        bool isWhite = IniHelper::getBool("white", tidStr, false, WHITE_INI_PATH);
        auto item = new tsl::elm::ToggleListItem(tidStr, isWhite);
        item->setStateChangedListener([tidStr = std::string(tidStr)](bool state) {
            if (state) IniHelper::setBool("white", tidStr, true, WHITE_INI_PATH);
            else IniHelper::removeKey("white", tidStr, WHITE_INI_PATH);
            g_ipcManager.sendReloadWhitelistCommand();
            GameMonitor::loadWhitelist();
        });
        entry.item = item;
        list->addItem(item);
    }

    frame->setContent(list);
    return frame;
}

void SettingWhitelist::update() {
    if (m_nextIndex >= m_apps.size()) return;
    auto& entry = m_apps[m_nextIndex];
    if (!entry.item) {
        m_nextIndex++;
        return;
    }
    char nameBuf[64]{};
    if (GameMonitor::getTitleIdGameName(entry.tid, nameBuf)) {
        entry.item->setText(nameBuf);
    }
    m_nextIndex++;
}

bool SettingWhitelist::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    
    if (keysDown & HidNpadButton_B || keysDown & HidNpadButton_Left) {
        tsl::goBack();
        return true;
    }

    return false;
}
