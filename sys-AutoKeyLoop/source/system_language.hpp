#pragma once
#include <switch.h>

// 定义全局变量
const char* g_NOTIF_AUTOFIRE_ON = "连发功能已开启";
const char* g_NOTIF_AUTOFIRE_OFF = "连发功能已关闭";
const char* g_NOTIF_MAPPING_ON = "映射功能已开启";
const char* g_NOTIF_MAPPING_OFF = "映射功能已关闭";
const char* g_NOTIF_ALL_ON = "全部功能已开启";
const char* g_NOTIF_ALL_OFF = "全部功能已关闭";


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
            g_NOTIF_MAPPING_ON = "映射功能已啟用";
            g_NOTIF_MAPPING_OFF = "映射功能已停用";
            g_NOTIF_ALL_ON = "全部功能已啟用";
            g_NOTIF_ALL_OFF = "全部功能已停用";
            break;
        case SetLanguage_JA:
            g_NOTIF_AUTOFIRE_ON = "連射機能有効";
            g_NOTIF_AUTOFIRE_OFF = "連射機能無効";
            g_NOTIF_MAPPING_ON = "配置機能有効";
            g_NOTIF_MAPPING_OFF = "配置機能無効";
            g_NOTIF_ALL_ON = "全機能有効";
            g_NOTIF_ALL_OFF = "全機能無効";
            break;
        case SetLanguage_KO:
            g_NOTIF_AUTOFIRE_ON = "연사 기능 활성화";
            g_NOTIF_AUTOFIRE_OFF = "연사 기능 해제됨";
            g_NOTIF_MAPPING_ON = "매핑 기능 활성화";
            g_NOTIF_MAPPING_OFF = "매핑 기능 해제됨";
            g_NOTIF_ALL_ON = "전체 기능 활성화";
            g_NOTIF_ALL_OFF = "전체 기능 해제됨";
            break;
        case SetLanguage_FR:
        case SetLanguage_FRCA:
            g_NOTIF_AUTOFIRE_ON = "Turbo Actif";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inactif";
            g_NOTIF_MAPPING_ON = "Mapping Actif";
            g_NOTIF_MAPPING_OFF = "Mapping Inactif";
            g_NOTIF_ALL_ON = "Tout Actif";
            g_NOTIF_ALL_OFF = "Tout Inactif";
            break;
        case SetLanguage_DE:
            g_NOTIF_AUTOFIRE_ON = "Turbo Aktiv";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inaktiv";
            g_NOTIF_MAPPING_ON = "Mapping Aktiv";
            g_NOTIF_MAPPING_OFF = "Mapping Inaktiv";
            g_NOTIF_ALL_ON = "Alles Aktiv";
            g_NOTIF_ALL_OFF = "Alles Inaktiv";
            break;
        case SetLanguage_IT:
            g_NOTIF_AUTOFIRE_ON = "Turbo Attivo";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inattivo";
            g_NOTIF_MAPPING_ON = "Mapping Attivo";
            g_NOTIF_MAPPING_OFF = "Mapping Inattivo";
            g_NOTIF_ALL_ON = "Tutto Attivo";
            g_NOTIF_ALL_OFF = "Tutto Inattivo";
            break;
        case SetLanguage_ES:
        case SetLanguage_ES419:
            g_NOTIF_AUTOFIRE_ON = "Turbo Activo";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inactivo";
            g_NOTIF_MAPPING_ON = "Mapeo Activo";
            g_NOTIF_MAPPING_OFF = "Mapeo Inactivo";
            g_NOTIF_ALL_ON = "Todo Activo";
            g_NOTIF_ALL_OFF = "Todo Inactivo";
            break;
        case SetLanguage_PT:
        case SetLanguage_PTBR:
            g_NOTIF_AUTOFIRE_ON = "Turbo Ativo";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inativo";
            g_NOTIF_MAPPING_ON = "Mapeamento Ativo";
            g_NOTIF_MAPPING_OFF = "Mapeamento Inativo";
            g_NOTIF_ALL_ON = "Tudo Ativo";
            g_NOTIF_ALL_OFF = "Tudo Inativo";
            break;
        case SetLanguage_RU:
            g_NOTIF_AUTOFIRE_ON = "Турбо Включен";
            g_NOTIF_AUTOFIRE_OFF = "Турбо Выключен";
            g_NOTIF_MAPPING_ON = "Маппинг Включен";
            g_NOTIF_MAPPING_OFF = "Маппинг Выключен";
            g_NOTIF_ALL_ON = "Всё Включено";
            g_NOTIF_ALL_OFF = "Всё Выключено";
            break;
        case SetLanguage_NL:
            g_NOTIF_AUTOFIRE_ON = "Turbo Actief";
            g_NOTIF_AUTOFIRE_OFF = "Turbo Inactief";
            g_NOTIF_MAPPING_ON = "Mapping Actief";
            g_NOTIF_MAPPING_OFF = "Mapping Inactief";
            g_NOTIF_ALL_ON = "Alles Actief";
            g_NOTIF_ALL_OFF = "Alles Inactief";
            break;
        default:
            g_NOTIF_AUTOFIRE_ON = "Turbo ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo OFF";
            g_NOTIF_MAPPING_ON = "Mapping ON";
            g_NOTIF_MAPPING_OFF = "Mapping OFF";
            g_NOTIF_ALL_ON = "All ON";
            g_NOTIF_ALL_OFF = "All OFF";
            break;
    }
}
