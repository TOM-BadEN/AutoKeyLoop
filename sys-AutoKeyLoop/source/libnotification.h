/**
 * @file libnotification.h
 * @brief sys-Notification 系统模块方便调用的 C 库
 * @date 2025
 * @author TOM
 * 
 * Header-Only C 库，用于向 sys-Notification 系统模块请求向switch发送弹窗
 * 
 * @section 依赖
 * - sys-Notification系统模块
 * - pmdmnt 服务（检查系统模块状态）
 * - pmshell 服务（启动系统模块）
 * 
 * @section 使用示例
 * @code
 * #include <switch.h>
 * #include "libnotification.h"
 * 
 * int main(void) {
 *     // 初始化服务
 *     pmdmntInitialize();
 *     pmshellInitialize();
 *     
 *     // 发送通知
 *     createNotification("Hello World!", 3, INFO, RIGHT);
 *     
 *     // 清理
 *     pmshellExit();
 *     pmdmntExit();
 *     return 0;
 * }
 * @endcode
 * 
 * @note 注意
 * - 该库为Header-Only库，只需包含头文件即可使用
 * - 使用很简单，也有example
 * - example是为了方便所以用的nro，实际上nro根本不需要三方的通知弹窗
 * - 这个系统模块主要是为了方便别的系统模块插件的调用，请勿滥用
 * 
 * @note 注意
 * - 系统模块必须安装
 * - 系统模块完成弹窗后会自动关闭
 * - 系统模块运行时内存占用1.33MB
 * - 因此如果你的插件需要使用该模块，请向用户提供配置关闭的功能
 * - 因为覆盖层全局只能有一个，所以无法与特斯拉覆盖层共存
 * - 当系统模块运行时，特斯拉会暂时隐藏，且无法打开，弹窗结束后，特斯拉会恢复显示
 * - 因此请不要滥用弹窗，尤其是特斯拉插件，你可以使用libultrahand的内置弹窗功能
 * - 否则会导致用户点一下，出现弹窗，特斯拉暂时无法操作的情况
 * - 弹窗最大持续时间为10s，但是非必要请尽量缩短时间
 * - 弹窗文本最大长度为7个中文字符，超过会被截断
 * - 文本支持unicode字符，和特斯拉一样的用法，对照表见目录
 */

#ifndef LIBNOTIFICATION_H
#define LIBNOTIFICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

// sys-Notification 的 Program ID
#define NOTIF_SYSMODULE_TID 0x0100000000251020ULL

// 通知配置目录路径
#define _NOTIF_CONFIG_DIR "/config/sys-Notification"

// 通知配置文件路径前缀
#define _NOTIF_FILE_PREFIX "/config/sys-Notification/notif_"

/**
 * @brief 通知类型
 */
typedef enum {
    INFO = 0,    // 信息
    ERROR = 1    // 错误
} NotificationType;

/**
 * @brief 通知位置
 */
typedef enum {
    LEFT = 0,    // 左对齐
    MIDDLE = 1,  // 居中
    RIGHT = 2    // 右对齐
} NotificationPosition;

/**
 * @brief 检查系统模块是否正在运行
 * @param program_id 系统模块的 Program ID
 * @return true 正在运行, false 未运行
 * @warning 这是内部函数，用户不应直接调用
 */
static inline bool _notif_is_running(u64 program_id) {
    u64 pid = 0;
    if (R_FAILED(pmdmntGetProcessId(&pid, program_id)))
        return false;
    return pid > 0;
}

/**
 * @brief 启动系统模块
 * @param program_id 系统模块的 Program ID
 * @return Result 结果码
 * @warning 这是内部函数，用户不应直接调用
 */
static inline Result _notif_launch(u64 program_id) {
    NcmProgramLocation location = {
        .program_id = program_id,
        .storageID = NcmStorageId_None,
    };
    u64 pid = 0;
    return pmshellLaunchProgram(0, &location, &pid);
}

/**
 * @brief 确保系统模块正在运行（如果未运行则启动）
 * @return Result 0=成功（已运行或成功启动），负数=错误码
 * @warning 这是内部函数，用户不应直接调用
 */
static inline Result _notif_ensure_running(void) {
    // 检查是否已运行
    if (_notif_is_running(NOTIF_SYSMODULE_TID)) {
        return 0;  // 已经在运行
    }
    
    // 启动系统模块
    Result rc = _notif_launch(NOTIF_SYSMODULE_TID);
    if (R_FAILED(rc)) {
        return rc;  // 启动失败
    }
    
    return 0;
}

/**
 * @brief 确保配置目录存在（如果不存在则创建）
 * @return true 成功, false 失败
 * @warning 这是内部函数，用户不应直接调用
 */
static inline bool _notif_ensure_dir(void) {
    // 检查目录是否存在
    DIR* dir = opendir(_NOTIF_CONFIG_DIR);
    if (dir) {
        closedir(dir);
        return true;  // 目录已存在
    }
    
    // 目录不存在，尝试创建
    if (mkdir(_NOTIF_CONFIG_DIR, 0755) == 0) {
        return true;  // 创建成功
    }
    
    // 如果错误是"已存在"，也算成功（处理竞态条件）
    if (errno == EEXIST) {
        return true;
    }
    
    return false;  // 创建失败
}

/**
 * @brief 生成随机通知配置文件路径（临时文件）
 * @param out_path 输出：文件路径缓冲区
 * @param size 缓冲区大小
 * @warning 这是内部函数，用户不应直接调用
 */
static inline void _notif_random_path(char* out_path, size_t size) {
    static bool initialized = false;
    
    if (!initialized) {
        // 使用系统时钟作为种子
        srand((unsigned int)armGetSystemTick());
        initialized = true;
    }
    
    u32 random = (u32)(rand() % 100000000);  // 限制到 8 位数（0-99999999）
    snprintf(out_path, size, "%s%u.ini.temp", _NOTIF_FILE_PREFIX, random);
}

/**
 * @brief 发送通知
 * @param text 通知文本
 * @param duration 显示时长（秒，范围 1-10）
 * @param type 通知类型 (INFO / ERROR)
 * @param position 通知位置 (LEFT / MIDDLE / RIGHT)
 * @return Result 0=成功，负数=失败
 */
static inline Result createNotification(const char* text, 
                                        int duration,
                                        NotificationType type, 
                                        NotificationPosition position) {
    // 检查参数
    if (!text || text[0] == '\0') return -1;
    
    // 校正 duration 范围（1-10 秒）
    if (duration < 1) duration = 1;
    else if (duration > 10) duration = 10;
    
    // 校正 type 枚举有效性
    if (type != INFO && type != ERROR) type = INFO;
    
    // 校正 position 枚举有效性
    if (position != LEFT && position != MIDDLE && position != RIGHT) position = RIGHT;

    // 清理文本：截断到 31 字符，替换换行符为空格
    char clean_text[32];
    strncpy(clean_text, text, 31);
    clean_text[31] = '\0';

    for (char* p = clean_text; *p; p++) {
        if (*p == '\n' || *p == '\r') *p = ' ';
    }

    // 转换枚举为字符串
    const char* type_str = (type == INFO) ? "INFO" : "ERROR";
    const char* pos_str = (position == LEFT) ? "LEFT" : 
                          (position == MIDDLE) ? "MIDDLE" : "RIGHT";
    
    // 确保配置目录存在
    if (!_notif_ensure_dir()) return -2;
    
    // 生成随机临时文件路径
    char temp_path[256];
    _notif_random_path(temp_path, sizeof(temp_path));
    
    // 写入临时文件
    FILE* f = fopen(temp_path, "w");
    if (!f) return -3;
    
    // 写入文件内容失败，删除临时文件
    if (fprintf(f, "text=%s\ntype=%s\nposition=%s\nduration=%d\n", 
                clean_text, type_str, pos_str, duration) < 0) {
        fclose(f);
        remove(temp_path);
        return -3;
    }
    
    // 关闭文件失败，删除临时文件
    if (fclose(f) != 0) {
        remove(temp_path);
        return -3;
    }
    
    // 生成最终文件路径（去掉 .temp 后缀）
    char final_path[256];
    strncpy(final_path, temp_path, sizeof(final_path) - 1);
    final_path[sizeof(final_path) - 1] = '\0';
    
    char* suffix = strstr(final_path, ".temp");
    if (suffix) *suffix = '\0';
    
    // 原子重命名
    if (rename(temp_path, final_path) != 0) {
        // 重命名失败，删除临时文件
        remove(temp_path);
        return -4;
    }

    // 如果系统模块未在运行则启动
    Result rc = _notif_ensure_running();
    if (R_FAILED(rc)) return rc;
    
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif // LIBNOTIFICATION_H

