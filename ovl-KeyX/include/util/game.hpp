#pragma once

#include <memory>
#include <cstring>
#include <switch.h>

/**
 * 游戏监控类 - 提供游戏相关的监控功能
 */
class GameMonitor {
public:
    /**
     * 获取当前运行程序的Title ID
     * @return 当前程序的Title ID字符串 (16位十六进制)
     */
    static u64 getCurrentTitleId();

    /**
     * 通过Title ID字符串获取程序名称
     * @param titleIdStr 程序的Title ID字符串
     * @param result 用于存储结果的字符数组指针
     * @return 如果找到游戏名称返回true，否则返回false（result中会包含"未知游戏"）
     */
    static bool getTitleIdGameName(u64 titleId, char* result);
};