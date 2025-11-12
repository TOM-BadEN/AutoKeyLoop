#include "setting_turbo.hpp"
#include "setting_turbo_config.hpp"
#include "game_monitor.hpp"
#include "ini_helper.hpp"
#include "ipc_manager.hpp"

// 配置文件路径常量
constexpr const char* CONFIG_PATH = "/config/KeyX/config.ini";

SettingTurbo::SettingTurbo() {
    m_defaultAuto = IniHelper::getBool("AUTOFIRE", "autoenable", false, CONFIG_PATH);
}

tsl::elm::Element* SettingTurbo::createUI() {
    auto frame = new tsl::elm::OverlayFrame("连发设置", "配置连发功能");
    auto list = new tsl::elm::List();
    
    auto ItemBasicKeySetting = new tsl::elm::CategoryHeader(" 调整连发参数配置");
    list->addItem(ItemBasicKeySetting);

    auto listItemGlobalSetting = new tsl::elm::ListItem("全局配置", ">");
    listItemGlobalSetting->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<SettingTurboConfig>(true, 0);
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
            tsl::changeTo<SettingTurboConfig>(false, currentTitleId);
            return true;
        }
        return false;
    });
    list->addItem(listItemIndependentSetting);

    auto ItemBasicSetting = new tsl::elm::CategoryHeader(" 基础功能设置");
    list->addItem(ItemBasicSetting);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  开启后自动启用连发功能", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);

    auto listItemDefaultAuto = new tsl::elm::ListItem("默认连发", m_defaultAuto ? "开" : "关");
    listItemDefaultAuto->setClickListener([listItemDefaultAuto, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_defaultAuto = !m_defaultAuto;
            IniHelper::setBool("AUTOFIRE", "autoenable", m_defaultAuto, CONFIG_PATH);
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
        renderer->drawString("  连发功能已适配全部机型，欢迎使用", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  非LITE机型的连发体验可能不够丝滑", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  已尽全力但底层限制，LITE连发完美", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);
    
    frame->setContent(list);
    return frame;
}

