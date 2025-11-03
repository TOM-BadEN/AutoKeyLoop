#include "setting/setting_remap.hpp"
#include "setting/setting_remap_config.hpp"
#include "game_monitor.hpp"
#include "ini_helper.hpp"
#include "ipc_manager.hpp"

// 配置文件路径常量
constexpr const char* CONFIG_PATH = "/config/AutoKeyLoop/config.ini";

SettingRemap::SettingRemap() {
    m_defaultAuto = IniHelper::getBool("MAPPING", "autoenable", false, CONFIG_PATH);
}

tsl::elm::Element* SettingRemap::createUI() {
    auto frame = new tsl::elm::OverlayFrame("按键映射", "配置映射功能");
    auto list = new tsl::elm::List();
    
    auto ItemRemapKeySetting = new tsl::elm::CategoryHeader(" 调整需要映射的按键");
    list->addItem(ItemRemapKeySetting);

    auto listItemGlobalSetting = new tsl::elm::ListItem("全局配置", ">");
    listItemGlobalSetting->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<SettingRemapConfig>(true, 0);
            return true;
        }
        return false;
    });
    list->addItem(listItemGlobalSetting);

    u64 currentTitleId = GameMonitor::getCurrentTitleId();
    auto listItemIndependentSetting = new tsl::elm::ListItem("独立配置", currentTitleId == 0 ? "\uE14C" : ">");
    listItemIndependentSetting->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            u64 currentTitleId = GameMonitor::getCurrentTitleId();
            if (currentTitleId == 0) return true;
            tsl::changeTo<SettingRemapConfig>(false, currentTitleId);
            return true;
        }
        return false;
    });
    list->addItem(listItemIndependentSetting);

    auto ItemBasicSetting = new tsl::elm::CategoryHeader(" 基础功能设置");
    list->addItem(ItemBasicSetting);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  开启后自动启用映射功能", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);

    auto listItemDefaultAuto = new tsl::elm::ListItem("默认映射", m_defaultAuto ? "开" : "关");
    listItemDefaultAuto->setClickListener([listItemDefaultAuto, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_defaultAuto = !m_defaultAuto;
            IniHelper::setBool("MAPPING", "autoenable", m_defaultAuto, CONFIG_PATH);
            g_ipcManager.sendReloadBasicCommand();
            listItemDefaultAuto->setValue(m_defaultAuto ? "开" : "关");
            return true;
        }
        return false;
    });
    list->addItem(listItemDefaultAuto);

    auto ItemWarningSetting = new tsl::elm::CategoryHeader(" 注意事项");
    list->addItem(ItemWarningSetting);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  映射功能不占用任何内存", false, x + 5, y + 20-7, 16, (tsl::warningTextColor));
    }), 30);

    
    frame->setContent(list);
    return frame;
}


