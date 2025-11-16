#pragma once
#include <switch.h>

// 定义全局变量
const char* g_NOTIF_AUTOFIRE_ON = "连发或宏已开启";
const char* g_NOTIF_AUTOFIRE_OFF = "连发或宏已关闭";
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
            g_NOTIF_AUTOFIRE_ON = "連發或巨集已啟用";
            g_NOTIF_AUTOFIRE_OFF = "連發或巨集已停用";
            g_NOTIF_MAPPING_ON = "映射功能已啟用";
            g_NOTIF_MAPPING_OFF = "映射功能已停用";
            g_NOTIF_ALL_ON = "全部功能已啟用";
            g_NOTIF_ALL_OFF = "全部功能已停用";
            break;
        case SetLanguage_JA:
            g_NOTIF_AUTOFIRE_ON = "連射/マクロ有効";
            g_NOTIF_AUTOFIRE_OFF = "連射/マクロ無効";
            g_NOTIF_MAPPING_ON = "配置機能有効";
            g_NOTIF_MAPPING_OFF = "配置機能無効";
            g_NOTIF_ALL_ON = "全機能有効";
            g_NOTIF_ALL_OFF = "全機能無効";
            break;
        case SetLanguage_KO:
            g_NOTIF_AUTOFIRE_ON = "연사/매크로 활성화";
            g_NOTIF_AUTOFIRE_OFF = "연사/매크로 해제됨";
            g_NOTIF_MAPPING_ON = "매핑 기능 활성화";
            g_NOTIF_MAPPING_OFF = "매핑 기능 해제됨";
            g_NOTIF_ALL_ON = "전체 기능 활성화";
            g_NOTIF_ALL_OFF = "전체 기능 해제됨";
            break;
        case SetLanguage_FR:
        case SetLanguage_FRCA:
            g_NOTIF_AUTOFIRE_ON = "Turbo/Macro ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo/Macro OFF";
            g_NOTIF_MAPPING_ON = "Mapping ON";
            g_NOTIF_MAPPING_OFF = "Mapping OFF";
            g_NOTIF_ALL_ON = "All ON";
            g_NOTIF_ALL_OFF = "All OFF";
            break;
        case SetLanguage_DE:
            g_NOTIF_AUTOFIRE_ON = "Turbo/Macro ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo/Macro OFF";
            g_NOTIF_MAPPING_ON = "Mapping ON";
            g_NOTIF_MAPPING_OFF = "Mapping OFF";
            g_NOTIF_ALL_ON = "All ON";
            g_NOTIF_ALL_OFF = "All OFF";
            break;
        case SetLanguage_IT:
            g_NOTIF_AUTOFIRE_ON = "Turbo/Macro ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo/Macro OFF";
            g_NOTIF_MAPPING_ON = "Mapping ON";
            g_NOTIF_MAPPING_OFF = "Mapping OFF";
            g_NOTIF_ALL_ON = "All ON";
            g_NOTIF_ALL_OFF = "All OFF";
            break;
        case SetLanguage_ES:
        case SetLanguage_ES419:
            g_NOTIF_AUTOFIRE_ON = "Turbo/Macro ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo/Macro OFF";
            g_NOTIF_MAPPING_ON = "Mapping ON";
            g_NOTIF_MAPPING_OFF = "Mapping OFF";
            g_NOTIF_ALL_ON = "All ON";
            g_NOTIF_ALL_OFF = "All OFF";
            break;
        case SetLanguage_PT:
        case SetLanguage_PTBR:
            g_NOTIF_AUTOFIRE_ON = "Turbo/Macro ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo/Macro OFF";
            g_NOTIF_MAPPING_ON = "Mapping ON";
            g_NOTIF_MAPPING_OFF = "Mapping OFF";
            g_NOTIF_ALL_ON = "All ON";
            g_NOTIF_ALL_OFF = "All OFF";
            break;
        case SetLanguage_RU:
            g_NOTIF_AUTOFIRE_ON = "Турбо/Макрос Включен";
            g_NOTIF_AUTOFIRE_OFF = "Турбо/Макрос Выключен";
            g_NOTIF_MAPPING_ON = "Маппинг Включен";
            g_NOTIF_MAPPING_OFF = "Маппинг Выключен";
            g_NOTIF_ALL_ON = "Всё Включено";
            g_NOTIF_ALL_OFF = "Всё Выключено";
            break;
        case SetLanguage_NL:
            g_NOTIF_AUTOFIRE_ON = "Turbo/Macro ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo/Macro OFF";
            g_NOTIF_MAPPING_ON = "Mapping ON";
            g_NOTIF_MAPPING_OFF = "Mapping OFF";
            g_NOTIF_ALL_ON = "All ON";
            g_NOTIF_ALL_OFF = "All OFF";
            break;
        default:
            g_NOTIF_AUTOFIRE_ON = "Turbo/Macro ON";
            g_NOTIF_AUTOFIRE_OFF = "Turbo/Macro OFF";
            g_NOTIF_MAPPING_ON = "Mapping ON";
            g_NOTIF_MAPPING_OFF = "Mapping OFF";
            g_NOTIF_ALL_ON = "All ON";
            g_NOTIF_ALL_OFF = "All OFF";
            break;
    }
}
