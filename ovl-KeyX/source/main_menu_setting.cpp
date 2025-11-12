#include "main_menu_setting.hpp"
#include "setting_turbo.hpp"
#include "setting_remap.hpp"
#include "setting_macro.hpp"
#include "ini_helper.hpp"
#include "ipc.hpp"
#include "sysmodule.hpp"
#include "main_menu.hpp"

// 配置文件路径常量
constexpr const char* CONFIG_PATH = "/config/KeyX/config.ini";

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

    auto listItemMacro = new tsl::elm::ListItem("脚本设置", ">");
    listItemMacro->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<SettingMacro>();
            return true;
        }
        return false;
    });
    list->addItem(listItemMacro);

    auto ItemBasicSetting = new tsl::elm::CategoryHeader(" 基础功能设置");
    list->addItem(ItemBasicSetting);

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

bool SettingMenu::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, 
    HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    
    // 监控 B 键返回，每次返回前刷新主页数据
    if (keysDown & HidNpadButton_B) {
        MainMenu::UpdateMainMenu();
    }

    if (keysDown & HidNpadButton_Left) {
        MainMenu::UpdateMainMenu();
        tsl::goBack();
        return true;
    }
    
    return false;  // 让默认的返回逻辑继续执行
}

