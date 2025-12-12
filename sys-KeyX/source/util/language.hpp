#pragma once
#include <switch.h>
#include <string>

// 定义全局变量
inline std::string g_NOTIF_Tubro = "连发";
inline std::string g_NOTIF_Macro = "宏";
inline std::string g_NOTIF_Mapping = "映射";
inline std::string g_NOTIF_All = "全部功能";
inline std::string g_NOTIF_On = "已开启";
inline std::string g_NOTIF_Off = "已关闭";
inline std::string g_NOTIF_And = "与";


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
            g_NOTIF_And = "與";
            g_NOTIF_Off = "已停用";
            g_NOTIF_On = "已啟用";
            g_NOTIF_Tubro = "連發";
            g_NOTIF_Macro = "巨集";
            g_NOTIF_Mapping = "映射";
            g_NOTIF_All = "全部功能";
            break;
        case SetLanguage_JA:
            g_NOTIF_And = "と";
            g_NOTIF_Off = "無効";
            g_NOTIF_On = "有効";
            g_NOTIF_Tubro = "連射";
            g_NOTIF_Macro = "マクロ";
            g_NOTIF_Mapping = "割当";
            g_NOTIF_All = "全機能";
            break;
        case SetLanguage_KO:
            g_NOTIF_And = " 및 ";
            g_NOTIF_Off = " 해제";
            g_NOTIF_On = " 적용";
            g_NOTIF_Tubro = "연사";
            g_NOTIF_Macro = "매크로";
            g_NOTIF_Mapping = "할당";
            g_NOTIF_All = "전체";
            break;
        case SetLanguage_FR:
        case SetLanguage_FRCA:
            g_NOTIF_And = " et ";
            g_NOTIF_Off = " Off";
            g_NOTIF_On = " On";
            g_NOTIF_Tubro = "Tubro";
            g_NOTIF_Macro = "Macro";
            g_NOTIF_Mapping = "Map";
            g_NOTIF_All = "Tout";
            break;
        case SetLanguage_DE:
            g_NOTIF_And = " und ";
            g_NOTIF_Off = " Aus";
            g_NOTIF_On = " An";
            g_NOTIF_Tubro = "Tubro";
            g_NOTIF_Macro = "Macro";
            g_NOTIF_Mapping = "Belegung";
            g_NOTIF_All = "Alle";
            break;
        case SetLanguage_IT:
            g_NOTIF_And = " e ";
            g_NOTIF_Off = " Off";
            g_NOTIF_On = " On";
            g_NOTIF_Tubro = "Tubro";
            g_NOTIF_Macro = "Macro";
            g_NOTIF_Mapping = "Asseg";
            g_NOTIF_All = "Tutto";
            break;
        case SetLanguage_ES:
        case SetLanguage_ES419:
            g_NOTIF_And = " y ";
            g_NOTIF_Off = " Off";
            g_NOTIF_On = " On";
            g_NOTIF_Tubro = "Tubro";
            g_NOTIF_Macro = "Macro";
            g_NOTIF_Mapping = "Asig";
            g_NOTIF_All = "Todo";
            break;
        case SetLanguage_PT:
        case SetLanguage_PTBR:
            g_NOTIF_And = " e ";
            g_NOTIF_Off = " Off";
            g_NOTIF_On = " On";
            g_NOTIF_Tubro = "Tubro";
            g_NOTIF_Macro = "Macro";
            g_NOTIF_Mapping = "Atrib";
            g_NOTIF_All = "Tudo";
            break;
        case SetLanguage_RU:
            g_NOTIF_And = " и ";
            g_NOTIF_Off = " Выкл";
            g_NOTIF_On = " Вкл";
            g_NOTIF_Tubro = "Tubro";
            g_NOTIF_Macro = "Макро";
            g_NOTIF_Mapping = "Назн";
            g_NOTIF_All = "Все";
            break;
        case SetLanguage_NL:
            g_NOTIF_And = " en ";
            g_NOTIF_Off = " Uit";
            g_NOTIF_On = " Aan";
            g_NOTIF_Tubro = "Tubro";
            g_NOTIF_Macro = "Macro";
            g_NOTIF_Mapping = "Toew";
            g_NOTIF_All = "Alles";
            break;
        default:
            g_NOTIF_And = " and ";
            g_NOTIF_Off = " Off";
            g_NOTIF_On = " On";
            g_NOTIF_Tubro = "Tubro";
            g_NOTIF_Macro = "Macro";
            g_NOTIF_Mapping = "Map";
            g_NOTIF_All = "All";
            break;
    }
}
