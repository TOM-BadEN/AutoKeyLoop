#include "turbo_setting.hpp"
#include "turbo_config.hpp"
#include "game.hpp"
#include "ini_helper.hpp"
#include "ipc.hpp"
#include "refresh.hpp"

// 配置文件路径常量
constexpr const char* CONFIG_PATH = "/config/KeyX/config.ini";

SettingTurbo::SettingTurbo() {
    m_defaultAuto = IniHelper::getBool("AUTOFIRE", "defaultautoenable", false, CONFIG_PATH);
    m_isJCRightHand = IniHelper::getBool("AUTOFIRE", "IsJCRightHand", true, CONFIG_PATH);
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
    m_listIndependentSetting = new tsl::elm::ListItem("独立配置", currentTitleId == 0 ? "\uE14C" : ">");
    m_listIndependentSetting->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            u64 currentTitleId = GameMonitor::getCurrentTitleId();
            if (currentTitleId == 0) return true;
            tsl::changeTo<SettingTurboConfig>(false, currentTitleId);
            return true;
        }
        return false;
    });
    list->addItem(m_listIndependentSetting);

    auto ItemBasicSetting = new tsl::elm::CategoryHeader(" 基础功能设置");
    list->addItem(ItemBasicSetting);
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  自动启用连发（仅全局配置）", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);

    auto listItemDefaultAuto = new tsl::elm::ListItem("默认连发", m_defaultAuto ? "开" : "关");
    listItemDefaultAuto->setClickListener([listItemDefaultAuto, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_defaultAuto = !m_defaultAuto;
            IniHelper::setBool("AUTOFIRE", "defaultautoenable", m_defaultAuto, CONFIG_PATH);
            listItemDefaultAuto->setValue(m_defaultAuto ? "开" : "关");
            return true;
        }
        return false;
    });
    list->addItem(listItemDefaultAuto);

    list->addItem(new tsl::elm::CategoryHeader(" 限制单边手柄连发"));
    list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
        renderer->drawString("  只允许左边或右边手柄（仅JoyCon有效）", false, x + 5, y + 20-7, 16, (tsl::highlightColor2));
    }), 30);
    auto listRightOrLeftHand = new tsl::elm::ListItem("限制手柄", m_isJCRightHand ? "" : "");
    listRightOrLeftHand->setValueColor(m_isJCRightHand ? tsl::Color{0xF, 0x5, 0x5, 0xF} : tsl::Color{0x00, 0xDD, 0xFF, 0xFF});
    listRightOrLeftHand->setClickListener([listRightOrLeftHand, this](u64 keys) {
        if (keys & HidNpadButton_A) {
            m_isJCRightHand = !m_isJCRightHand;
            IniHelper::setBool("AUTOFIRE", "IsJCRightHand", m_isJCRightHand, CONFIG_PATH);
            listRightOrLeftHand->setValue(m_isJCRightHand ? "" : "");
            listRightOrLeftHand->setValueColor(m_isJCRightHand ? tsl::Color{0xF, 0x5, 0x5, 0xF} : tsl::Color{0x00, 0xDD, 0xFF, 0xFF});
            g_ipcManager.sendReloadAutoFireCommand();
            return true;
        }
        return false;
    });
    list->addItem(listRightOrLeftHand);
    
    frame->setContent(list);
    return frame;
}

void SettingTurbo::update() {
    if (Refresh::RefrConsume(Refresh::OnShow)) m_listIndependentSetting->setValue(GameMonitor::getCurrentTitleId() == 0 ? "\uE14C" : ">");
}


