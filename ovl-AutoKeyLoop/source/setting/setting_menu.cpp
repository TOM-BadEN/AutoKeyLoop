#include "setting/setting_menu.hpp"
#include "setting/setting_turbo.hpp"
#include "setting/setting_remap.hpp"
#include "ini_helper.hpp"
#include "ipc_manager.hpp"
#include "sysmodule_manager.hpp"

// 配置文件路径常量
constexpr const char* CONFIG_PATH = "/config/AutoKeyLoop/config.ini";

constexpr const char* const descriptions[2][2] = {
    [0] = {
        [0] = "关",
        [1] = "关",
    },
    [1] = {
        [0] = "开",
        [1] = "开",
    },
};

SettingMenu::SettingMenu() {
    m_notifEnabled = IniHelper::getBool("NOTIFICATION", "notif", false, CONFIG_PATH);
    m_running = SysModuleManager::isRunning();
    m_hasFlag = SysModuleManager::hasBootFlag();
}

tsl::elm::Element* SettingMenu::createUI() {
    auto frame = new tsl::elm::OverlayFrame("功能设置", "选择设置项");
    auto list = new tsl::elm::List();
    
    auto ItemBasicKeySetting = new tsl::elm::CategoryHeader(" 基础按键设置");
    list->addItem(ItemBasicKeySetting);

    auto listItemTurbo = new tsl::elm::ListItem("连发设置", ">");
    listItemTurbo->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<SettingTurbo>();
            return true;
        }
        return false;
    });
    list->addItem(listItemTurbo);

    auto listItemMapping = new tsl::elm::ListItem("映射设置", ">");
    listItemMapping->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<SettingRemap>();
            return true;
        }
        return false;
    });
    list->addItem(listItemMapping);

    auto ItemBasicSetting = new tsl::elm::CategoryHeader(" 基础功能设置");
    list->addItem(ItemBasicSetting);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  触发时额外占用内存 688KB", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);

    auto listItemNotif = new tsl::elm::ListItem("通知弹窗", m_notifEnabled ? "开" : "关");
    listItemNotif->setClickListener([listItemNotif, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_notifEnabled = !m_notifEnabled;
            IniHelper::setBool("NOTIFICATION", "notif", m_notifEnabled, CONFIG_PATH);
            g_ipcManager.sendReloadBasicCommand();
            listItemNotif->setValue(m_notifEnabled ? "开" : "关");
            return true;
        }
        return false;
    });
    list->addItem(listItemNotif);

    auto ItemModuleManager = new tsl::elm::CategoryHeader(" 功能模块管理 切换自启动 开关");
    list->addItem(ItemModuleManager);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  待机占用 450KB，连发时占用 706KB", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);

    auto listItemTurboModule = new tsl::elm::ListItem("系统模块", descriptions[m_running][m_hasFlag]);
    listItemTurboModule->setClickListener([listItemTurboModule, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            SysModuleManager::toggleModule();
            m_running = SysModuleManager::isRunning();
            listItemTurboModule->setValue(descriptions[m_running][m_hasFlag]);
            return true;
        }
        if (keys & HidNpadButton_Y) {
            SysModuleManager::toggleBootFlag();
            m_hasFlag = SysModuleManager::hasBootFlag();
            listItemTurboModule->setValue(descriptions[m_running][m_hasFlag]);
            return true;
        }
        return false;
    });
    list->addItem(listItemTurboModule);
        
    frame->setContent(list);
    return frame;
}

