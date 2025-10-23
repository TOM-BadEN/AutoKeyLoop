#pragma once
#include <switch.h>

// 定义全局变量
const char* g_NOTIF_AUTOFIRE_ON = "连发功能已开启";
const char* g_NOTIF_AUTOFIRE_OFF = "连发功能已关闭";

// 检测系统语言是否为中文
inline void getSetNotifLanguage() {
    u64 languageCode;
    SetLanguage setLanguage;
    
    if (R_FAILED(setGetSystemLanguage(&languageCode))) {
        return;
    }
    
    if (R_FAILED(setMakeLanguage(languageCode, &setLanguage))) {
        return;
    }
    
    switch (setLanguage) {
        case SetLanguage_ZHCN:
        case SetLanguage_ZHHANS:
            break;
        case SetLanguage_ZHTW:
        case SetLanguage_ZHHANT:
            g_NOTIF_AUTOFIRE_ON = "連發功能已啟用";
            g_NOTIF_AUTOFIRE_OFF = "連發功能已停用";
            break;
        case SetLanguage_JA:
            g_NOTIF_AUTOFIRE_ON = "連射機能有効";
            g_NOTIF_AUTOFIRE_OFF = "連射機能無効";
            break;
        case SetLanguage_KO:
            g_NOTIF_AUTOFIRE_ON = "연사 기능 활성화";
            g_NOTIF_AUTOFIRE_OFF = "연사 기능 해제됨";
            break;
        case SetLanguage_FR:
        case SetLanguage_FRCA:
            g_NOTIF_AUTOFIRE_ON = "Turbo Actif";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inactif";
            break;
        case SetLanguage_DE:
            g_NOTIF_AUTOFIRE_ON = "Turbo Aktiv";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inaktiv";
            break;
        case SetLanguage_IT:
            g_NOTIF_AUTOFIRE_ON = "Turbo Attivo";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inattivo";
            break;
        case SetLanguage_ES:
        case SetLanguage_ES419:
            g_NOTIF_AUTOFIRE_ON = "Turbo Activo";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inactivo";
            break;
        case SetLanguage_PT:
        case SetLanguage_PTBR:
            g_NOTIF_AUTOFIRE_ON = "Turbo Ativo";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inativo";
            break;
        case SetLanguage_RU:
            g_NOTIF_AUTOFIRE_ON = "Турбо Включен";
            g_NOTIF_AUTOFIRE_OFF = "Турбо Выключен";
            break;
        case SetLanguage_NL:
            g_NOTIF_AUTOFIRE_ON = "Turbo Actief";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inactief";
            break;
        default:
            g_NOTIF_AUTOFIRE_ON = "Turbo ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo OFF";
            break;
    }
}
