#include "macro_data.hpp"
#include "macro_util.hpp"
#include <cstdio>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>
#include <ultra.hpp>

// 静态成员定义
char MacroData::s_filePath[128];
MacroHeader MacroData::s_header;
MacroBasicInfo MacroData::s_basicInfo;
std::vector<MacroFrame> MacroData::s_frames;
std::vector<MacroFrameV2> MacroData::s_framesV2;
std::vector<Action> MacroData::s_actions;

// 撤销快照栈
std::vector<MacroData::UndoSnapshot> MacroData::s_undoStack;

// 摇杆死区 (约2%)
constexpr s32 STICK_DEADZONE = 655;

// 确定摇杆方向
StickDir MacroData::getStickDir(s32 x, s32 y) {
    bool hasRight = (x > STICK_DEADZONE);
    bool hasLeft  = (x < -STICK_DEADZONE);
    bool hasUp    = (y > STICK_DEADZONE);
    bool hasDown  = (y < -STICK_DEADZONE);
    
    if (hasUp && hasRight) return StickDir::UpRight;
    if (hasUp && hasLeft)  return StickDir::UpLeft;
    if (hasDown && hasRight) return StickDir::DownRight;
    if (hasDown && hasLeft)  return StickDir::DownLeft;
    if (hasUp)   return StickDir::Up;
    if (hasDown) return StickDir::Down;
    if (hasRight) return StickDir::Right;
    if (hasLeft)  return StickDir::Left;
    return StickDir::None;
}

// 对比两帧的数据是否相同
bool MacroData::isSameState(const MacroFrame& a, const MacroFrame& b) {
    if (a.keysHeld != b.keysHeld) return false;
    if (getStickDir(a.leftX, a.leftY) != getStickDir(b.leftX, b.leftY)) return false;
    if (getStickDir(a.rightX, a.rightY) != getStickDir(b.rightX, b.rightY)) return false;
    return true;
}

// 对比两帧的数据是否相同（V2 重载）
bool MacroData::isSameState(const MacroFrameV2& a, const MacroFrameV2& b) {
    if (a.keysHeld != b.keysHeld) return false;
    if (getStickDir(a.leftX, a.leftY) != getStickDir(b.leftX, b.leftY)) return false;
    if (getStickDir(a.rightX, a.rightY) != getStickDir(b.rightX, b.rightY)) return false;
    return true;
}

// 加载获取宏基础数据
bool MacroData::load(const char* filePath) {
    strncpy(s_filePath, filePath, sizeof(s_filePath) - 1);
    s_filePath[sizeof(s_filePath) - 1] = '\0';
    return loadFrameAndBasicInfo(filePath);
}

// 从备份文件加载宏数据
bool MacroData::loadBakMacroData() {
    char bakPath[128];
    snprintf(bakPath, sizeof(bakPath), "%s.bak", s_filePath);
    if (!loadFrameAndBasicInfo(bakPath)) return false;
    parseActions();
    return true;
}

// 加载帧数据和基础信息
bool MacroData::loadFrameAndBasicInfo(const char* filePath) {
    const char* path = filePath ? filePath : s_filePath;
    s_frames.clear();
    s_framesV2.clear();
    s_basicInfo = {};
    FILE* fp = fopen(path, "rb");
    if (!fp) return false;
    // 读取文件头
    fread(&s_header, sizeof(MacroHeader), 1, fp);
    // 获取所有帧数据
    if (s_header.version == 1) {
        s_frames.resize(s_header.frameCount);
        fread(s_frames.data(), sizeof(MacroFrame), s_header.frameCount, fp);
    } else {
        s_framesV2.resize(s_header.frameCount);
        fread(s_framesV2.data(), sizeof(MacroFrameV2), s_header.frameCount, fp);
    }
    fclose(fp);
    
    // 填充基础信息
    s_basicInfo.version = s_header.version;
    s_basicInfo.titleId = s_header.titleId;
    s_basicInfo.frameRate = s_header.frameRate;
    s_basicInfo.frameCount = s_header.frameCount;
    if (s_header.version == 1) s_basicInfo.durationMs = s_header.frameRate ? (s_header.frameCount * 1000 / s_header.frameRate) : 0;
    else for (const auto& frame : s_framesV2) s_basicInfo.durationMs += frame.durationMs;
    struct stat st{};
    if (stat(path, &st) == 0) s_basicInfo.fileSize = st.st_size;
    else s_basicInfo.fileSize = 0;
    // 获取显示名称（优先从元数据读取，否则用文件名）
    std::string pathStr = path;
    if (pathStr.size() > 4 && pathStr.substr(pathStr.size() - 4) == ".bak") {
        pathStr = pathStr.substr(0, pathStr.size() - 4);
    }
    std::string displayName = MacroUtil::getDisplayName(pathStr.c_str());
    strncpy(s_basicInfo.fileName, displayName.c_str(), sizeof(s_basicInfo.fileName) - 1);
    s_basicInfo.fileName[sizeof(s_basicInfo.fileName) - 1] = '\0';
    
    return true;
}

// 获取文件路径
const char* MacroData::getFilePath() {
    return s_filePath;
}

// 保存编辑后的宏数据（统一入口）
bool MacroData::saveForEdit() {
    // 先将原文件备份
    char bakPath[128];
    snprintf(bakPath, sizeof(bakPath), "%s.bak", s_filePath);
    ult::copyFileOrDirectory(s_filePath, bakPath);
    // 再修改
    FILE* fp = fopen(s_filePath, "wb");
    if (!fp) return false;
    if (s_header.version == 1) saveForEditV1(fp);
    else saveForEditV2(fp);
    fclose(fp);
    return true;
}

// V1: 保存帧数据
void MacroData::saveForEditV1(FILE* fp) {
    s_header.frameCount = s_frames.size();
    fwrite(&s_header, sizeof(MacroHeader), 1, fp);
    fwrite(s_frames.data(), sizeof(MacroFrame), s_frames.size(), fp);
}

// V2: 保存帧数据
void MacroData::saveForEditV2(FILE* fp) {
    
    // 合并相邻相同的帧（消除编辑产生的碎片）
    for (size_t i = 1; i < s_framesV2.size(); ) {
        auto& prev = s_framesV2[i - 1];
        auto& curr = s_framesV2[i];
        if (prev.keysHeld == curr.keysHeld && 
            prev.leftX == curr.leftX && prev.leftY == curr.leftY &&
            prev.rightX == curr.rightX && prev.rightY == curr.rightY) {
            prev.durationMs += curr.durationMs;
            s_framesV2.erase(s_framesV2.begin() + i);
        } else {
            i++;
        }
    }
    s_header.frameCount = s_framesV2.size();
    fwrite(&s_header, sizeof(MacroHeader), 1, fp);
    fwrite(s_framesV2.data(), sizeof(MacroFrameV2), s_framesV2.size(), fp);
}

// 获取文件头数据
const MacroHeader& MacroData::getHeader() {
    return s_header;
}

// 获取基础信息
const MacroBasicInfo& MacroData::getBasicInfo() {
    return s_basicInfo;
}

// 解析所有帧数据的动作（统一入口）
void MacroData::parseActions() {
    s_actions.clear();
    if (s_header.version == 1) parseActionsV1();
    else parseActionsV2();
}

// V1: 解析帧数据到动作列表
void MacroData::parseActionsV1() {
    if (s_frames.empty()) return;
    
    Action current;
    current.buttons = s_frames[0].keysHeld;
    current.stickL = getStickDir(s_frames[0].leftX, s_frames[0].leftY);
    current.stickR = getStickDir(s_frames[0].rightX, s_frames[0].rightY);
    current.frameCount = 1;
    current.stickMergedVirtual = false;
    
    for (size_t i = 1; i < s_frames.size(); i++) {
        const auto& cur = s_frames[i];
        const auto& prev = s_frames[i-1];
        
        // 判断是否是纯摇杆
        bool curIsPureStick = (cur.keysHeld == 0) && 
            (getStickDir(cur.leftX, cur.leftY) != StickDir::None || 
             getStickDir(cur.rightX, cur.rightY) != StickDir::None);
        bool prevIsPureStick = (prev.keysHeld == 0) && 
            (getStickDir(prev.leftX, prev.leftY) != StickDir::None || 
             getStickDir(prev.rightX, prev.rightY) != StickDir::None);
        
        bool canMerge = false;
        if (curIsPureStick && prevIsPureStick) {
            // 纯摇杆：八向相同即合并
            canMerge = isSameState(cur, prev);
            if (canMerge) current.stickMergedVirtual = true;
        } else if (!curIsPureStick && !prevIsPureStick) {
            // 非纯摇杆
            bool hasStick = (getStickDir(cur.leftX, cur.leftY) != StickDir::None || 
                             getStickDir(cur.rightX, cur.rightY) != StickDir::None);
            if (hasStick) {
                // 按键+摇杆：按键相同 + 八向相同即合并
                canMerge = (cur.keysHeld == prev.keysHeld && isSameState(cur, prev));
                if (canMerge) current.stickMergedVirtual = true;
            } else {
                // 纯按键：帧完全相同才合并
                canMerge = (cur.keysHeld == prev.keysHeld &&
                            cur.leftX == prev.leftX && cur.leftY == prev.leftY &&
                            cur.rightX == prev.rightX && cur.rightY == prev.rightY);
            }
        }
        // 类型不同：不合并
        
        if (canMerge) {
            current.frameCount++;
        } else {
            current.duration = current.frameCount * 1000 / s_header.frameRate;
            s_actions.push_back(current);
            current.buttons = cur.keysHeld;
            current.stickL = getStickDir(cur.leftX, cur.leftY);
            current.stickR = getStickDir(cur.rightX, cur.rightY);
            current.frameCount = 1;
            current.stickMergedVirtual = false;
        }
    }
    current.duration = current.frameCount * 1000 / s_header.frameRate;
    s_actions.push_back(current);
}

// V2: 解析帧数据到动作列表
void MacroData::parseActionsV2() {
    if (s_framesV2.empty()) return;
    
    Action current;
    bool inStickMerge = false;  // 正在累积摇杆（纯摇杆或按键+摇杆）
    
    for (size_t i = 0; i < s_framesV2.size(); i++) {
        const auto& frame = s_framesV2[i];
        
        bool hasStick = (getStickDir(frame.leftX, frame.leftY) != StickDir::None || 
                         getStickDir(frame.rightX, frame.rightY) != StickDir::None);
        
        if (hasStick) {
            // 有摇杆（纯摇杆或按键+摇杆）
            if (!inStickMerge) {
                // 开始新的摇杆累积
                current.buttons = frame.keysHeld;
                current.stickL = getStickDir(frame.leftX, frame.leftY);
                current.stickR = getStickDir(frame.rightX, frame.rightY);
                current.duration = frame.durationMs;
                current.frameCount = 1;
                current.stickMergedVirtual = false;
                inStickMerge = true;
            } else {
                // 继续累积，检查按键相同+八向相同
                if (frame.keysHeld == current.buttons && isSameState(frame, s_framesV2[i-1])) {
                    current.duration += frame.durationMs;
                    current.frameCount++;
                    current.stickMergedVirtual = true;
                } else {
                    // 不同，推入当前动作，开始新动作
                    s_actions.push_back(current);
                    current.buttons = frame.keysHeld;
                    current.stickL = getStickDir(frame.leftX, frame.leftY);
                    current.stickR = getStickDir(frame.rightX, frame.rightY);
                    current.duration = frame.durationMs;
                    current.frameCount = 1;
                    current.stickMergedVirtual = false;
                }
            }
        } else {
            // 无摇杆（纯按键或无动作，录制时已合并）
            if (inStickMerge) {
                s_actions.push_back(current);
                inStickMerge = false;
            }
            current.buttons = frame.keysHeld;
            current.stickL = StickDir::None;
            current.stickR = StickDir::None;
            current.duration = frame.durationMs;
            current.frameCount = 1;
            current.stickMergedVirtual = false;
            s_actions.push_back(current);
        }
    }
    
    // 循环结束，推入累积中的动作
    if (inStickMerge) {
        s_actions.push_back(current);
    }
}

// 获取所有动作
std::vector<Action>& MacroData::getActions() {
    return s_actions;
}

// 更新总时长
void MacroData::updateDurationMs() {
    s_basicInfo.durationMs = 0;
    for (const auto& action : s_actions) {
        s_basicInfo.durationMs += action.duration;
    }
}

// 插入一个无动作（100ms，不合并）
void MacroData::insertAction(s32 actionIndex, bool insertBefore) {
    saveSnapshot();
    s32 insertPos = insertBefore ? actionIndex : actionIndex + 1;
    
    // 计算帧插入位置
    s32 frameInsertPos = 0;
    for (s32 i = 0; i < insertPos && i < (s32)s_actions.size(); i++)
        frameInsertPos += s_actions[i].frameCount;
    
    if (s_header.version == 1) {
        // V1: 计算100ms对应的帧数
        u32 newFrameCount = (100 * s_header.frameRate + 500) / 1000;
        s_frames.insert(s_frames.begin() + frameInsertPos, newFrameCount, {});
        s_header.frameCount = s_frames.size();
        Action newAction = {0, StickDir::None, StickDir::None, 100, newFrameCount, true, false};
        s_actions.insert(s_actions.begin() + insertPos, newAction);
    } else {
        // V2: 直接100ms
        s_framesV2.insert(s_framesV2.begin() + frameInsertPos, {100, 0, 0, 0, 0, 0});
        s_header.frameCount++;
        Action newAction = {0, StickDir::None, StickDir::None, 100, 1, true, false};
        s_actions.insert(s_actions.begin() + insertPos, newAction);
    }
    
    updateDurationMs();
}

// 复制动作到指定位置
void MacroData::copyActions(s32 startIndex, s32 endIndex, bool insertBefore) {
    if (startIndex < 0 || endIndex >= (s32)s_actions.size() || startIndex > endIndex) return;
    
    saveSnapshot();
    
    // 计算源动作的帧范围
    s32 srcFrameStart = 0;
    for (s32 i = 0; i < startIndex; i++) srcFrameStart += s_actions[i].frameCount;
    u32 totalFrames = 0;
    for (s32 i = startIndex; i <= endIndex; i++) totalFrames += s_actions[i].frameCount;
    
    // 计算插入位置
    s32 insertPos = insertBefore ? startIndex : endIndex + 1;
    s32 insertFramePos = 0;
    for (s32 i = 0; i < insertPos; i++) insertFramePos += s_actions[i].frameCount;
    
    // 复制帧数据
    if (s_header.version == 1) {
        std::vector<MacroFrame> copiedFrames(s_frames.begin() + srcFrameStart, s_frames.begin() + srcFrameStart + totalFrames);
        s_frames.insert(s_frames.begin() + insertFramePos, copiedFrames.begin(), copiedFrames.end());
        s_header.frameCount = s_frames.size();
    } else {
        std::vector<MacroFrameV2> copiedFrames(s_framesV2.begin() + srcFrameStart, s_framesV2.begin() + srcFrameStart + totalFrames);
        s_framesV2.insert(s_framesV2.begin() + insertFramePos, copiedFrames.begin(), copiedFrames.end());
        s_header.frameCount = s_framesV2.size();
    }
    
    // 先保存要复制的动作（避免插入时索引偏移）
    std::vector<Action> copiedActions;
    for (s32 i = startIndex; i <= endIndex; i++) {
        Action copied = s_actions[i];
        copied.modified = true;
        copiedActions.push_back(copied);
    }
    
    // 再插入动作（不合并，保持独立）
    for (size_t i = 0; i < copiedActions.size(); i++) {
        s_actions.insert(s_actions.begin() + insertPos + i, copiedActions[i]);
    }
    
    updateDurationMs();
}

void MacroData::resetActions(s32 startIndex, s32 endIndex) {
    saveSnapshot();
    
    // 计算帧起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < startIndex; i++)
        frameStart += s_actions[i].frameCount;
    
    // 计算总帧数和V2总时长
    u32 totalFrames = 0;
    u32 totalDurationV2 = 0;
    for (s32 i = startIndex; i <= endIndex; i++) {
        totalFrames += s_actions[i].frameCount;
        totalDurationV2 += s_actions[i].duration;
    }
    
    if (s_header.version == 1) {
        // V1: 每帧清零，帧数量不变
        for (u32 i = 0; i < totalFrames; i++)
            s_frames[frameStart + i] = {};
        
        // 删除选中动作
        for (s32 i = endIndex; i >= startIndex; i--)
            s_actions.erase(s_actions.begin() + i);
        
        // 插入新无动作，duration从帧数重新计算
        u32 totalDuration = totalFrames * 1000 / s_header.frameRate;
        Action newAction = {0, StickDir::None, StickDir::None, totalDuration, totalFrames, true, false};
        s_actions.insert(s_actions.begin() + startIndex, newAction);
    } else {
        // V2: 合并成1帧，清零
        s_framesV2[frameStart] = {totalDurationV2, 0, 0, 0, 0, 0};
        if (totalFrames > 1) {
            s_framesV2.erase(s_framesV2.begin() + frameStart + 1, s_framesV2.begin() + frameStart + totalFrames);
            s_header.frameCount = s_framesV2.size();
        }
        
        // 删除选中动作
        for (s32 i = endIndex; i >= startIndex; i--)
            s_actions.erase(s_actions.begin() + i);
        
        // 插入新无动作
        Action newAction = {0, StickDir::None, StickDir::None, totalDurationV2, 1, true, false};
        s_actions.insert(s_actions.begin() + startIndex, newAction);
    }

    updateDurationMs();
}

// 删除动作内部实现（不保存快照、不合并、不更新时长）
void MacroData::deleteActionsInternal(s32 startIndex, s32 endIndex) {
    // 计算帧起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < startIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    // 计算要删除的总帧数
    u32 totalFrames = 0;
    for (s32 i = startIndex; i <= endIndex; i++) {
        totalFrames += s_actions[i].frameCount;
    }
    
    // 删除帧数据
    if (s_header.version == 1) {
        s_frames.erase(s_frames.begin() + frameStart, s_frames.begin() + frameStart + totalFrames);
        s_header.frameCount = s_frames.size();
    } else {
        s_framesV2.erase(s_framesV2.begin() + frameStart, s_framesV2.begin() + frameStart + totalFrames);
        s_header.frameCount = s_framesV2.size();
    }
    
    // 删除动作
    s_actions.erase(s_actions.begin() + startIndex, s_actions.begin() + endIndex + 1);
}

void MacroData::deleteActions(s32 startIndex, s32 endIndex) {
    saveSnapshot();
    deleteActionsInternal(startIndex, endIndex);
    updateDurationMs();
}

// 清除摇杆数据
void MacroData::clearStick(s32 startIndex, s32 endIndex, StickTarget target) {
    if (startIndex < 0 || endIndex >= (s32)s_actions.size() || startIndex > endIndex) return;
    
    saveSnapshot();
    
    // 从后往前遍历（避免删除后索引变化）
    for (s32 i = endIndex; i >= startIndex; i--) {
        // 检查是否有目标摇杆
        bool hasTargetStick = false;
        if (target == StickTarget::Both) {
            hasTargetStick = (s_actions[i].stickL != StickDir::None || s_actions[i].stickR != StickDir::None);
        } else if (target == StickTarget::Left) {
            hasTargetStick = (s_actions[i].stickL != StickDir::None);
        } else {
            hasTargetStick = (s_actions[i].stickR != StickDir::None);
        }
        
        if (!hasTargetStick) continue;
        
        // 清零摇杆坐标
        s32 frameStart = 0;
        for (s32 j = 0; j < i; j++) frameStart += s_actions[j].frameCount;
        
        for (u32 f = 0; f < s_actions[i].frameCount; f++) {
            if (s_header.version == 1) {
                if (target != StickTarget::Right) {
                    s_frames[frameStart + f].leftX = 0;
                    s_frames[frameStart + f].leftY = 0;
                }
                if (target != StickTarget::Left) {
                    s_frames[frameStart + f].rightX = 0;
                    s_frames[frameStart + f].rightY = 0;
                }
            } else {
                if (target != StickTarget::Right) {
                    s_framesV2[frameStart + f].leftX = 0;
                    s_framesV2[frameStart + f].leftY = 0;
                }
                if (target != StickTarget::Left) {
                    s_framesV2[frameStart + f].rightX = 0;
                    s_framesV2[frameStart + f].rightY = 0;
                }
            }
        }
        
        // 更新虚拟摇杆方向
        if (target != StickTarget::Right) s_actions[i].stickL = StickDir::None;
        if (target != StickTarget::Left) s_actions[i].stickR = StickDir::None;
        
        // 判断清零后是否还有另一边摇杆
        bool hasOtherStickAfterClear = (s_actions[i].stickL != StickDir::None || s_actions[i].stickR != StickDir::None);
        
        if (!hasOtherStickAfterClear && s_actions[i].frameCount > 1) {   // 没有另一边摇杆，需要合并
            if (s_header.version == 1) {  // V1：帧序列，不删帧，时间用帧数计算
                s_actions[i].duration = s_actions[i].frameCount * 1000 / s_header.frameRate;
            } else {   // V2：时间序列，累加时间到第1帧，删除多余帧
                u32 totalDuration = 0;
                for (u32 f = 0; f < s_actions[i].frameCount; f++) {
                    totalDuration += s_framesV2[frameStart + f].durationMs;
                }
                s_framesV2[frameStart].durationMs = totalDuration;
                s_framesV2.erase(s_framesV2.begin() + frameStart + 1, s_framesV2.begin() + frameStart + s_actions[i].frameCount);
                s_actions[i].frameCount = 1;
                s_actions[i].duration = totalDuration;
            }
            s_actions[i].stickMergedVirtual = false;
        }
        
        // 有另一边摇杆或只有1帧：不需要额外处理
        
        s_actions[i].modified = true;
    }
    
    updateDurationMs();
}

// 方向转坐标的辅助函数
static void dirToCoord(StickDir dir, s32& outX, s32& outY) {
    constexpr s32 STICK_MAX = 32767;
    switch (dir) {
        case StickDir::Up:        outX = 0;          outY = STICK_MAX;  break;
        case StickDir::Down:      outX = 0;          outY = -STICK_MAX; break;
        case StickDir::Left:      outX = -STICK_MAX; outY = 0;          break;
        case StickDir::Right:     outX = STICK_MAX;  outY = 0;          break;
        case StickDir::UpLeft:    outX = -STICK_MAX; outY = STICK_MAX;  break;
        case StickDir::UpRight:   outX = STICK_MAX;  outY = STICK_MAX;  break;
        case StickDir::DownLeft:  outX = -STICK_MAX; outY = -STICK_MAX; break;
        case StickDir::DownRight: outX = STICK_MAX;  outY = -STICK_MAX; break;
        default:                  outX = 0;          outY = 0;          break;
    }
}

// 设置动作摇杆方向
void MacroData::setActionStick(s32 actionIndex, StickDir leftDir, StickDir rightDir) {
    if (actionIndex < 0 || actionIndex >= (s32)s_actions.size()) return;
    
    saveSnapshot();
    
    // 计算帧范围
    s32 frameStart = 0;
    for (s32 i = 0; i < actionIndex; i++) frameStart += s_actions[i].frameCount;
    
    // 转换方向为坐标
    s32 lx, ly, rx, ry;
    dirToCoord(leftDir, lx, ly);
    dirToCoord(rightDir, rx, ry);
    
    if (s_header.version == 1) {
        // V1：遍历所有帧设置坐标
        for (u32 f = 0; f < s_actions[actionIndex].frameCount; f++) {
            s_frames[frameStart + f].leftX = lx;
            s_frames[frameStart + f].leftY = ly;
            s_frames[frameStart + f].rightX = rx;
            s_frames[frameStart + f].rightY = ry;
        }
    } else {
        // V2
        if (s_actions[actionIndex].stickMergedVirtual) {
            // 虚拟合并：合并多帧为1帧
            u32 totalDuration = 0;
            for (u32 f = 0; f < s_actions[actionIndex].frameCount; f++)
                totalDuration += s_framesV2[frameStart + f].durationMs;
            
            // 设置第一帧
            s_framesV2[frameStart].leftX = lx;
            s_framesV2[frameStart].leftY = ly;
            s_framesV2[frameStart].rightX = rx;
            s_framesV2[frameStart].rightY = ry;
            s_framesV2[frameStart].durationMs = totalDuration;
            
            // 删除多余帧
            if (s_actions[actionIndex].frameCount > 1) {
                s_framesV2.erase(s_framesV2.begin() + frameStart + 1,
                                 s_framesV2.begin() + frameStart + s_actions[actionIndex].frameCount);
                s_header.frameCount = s_framesV2.size();
            }
            s_actions[actionIndex].frameCount = 1;
        } else {
            // 非虚拟合并：直接设置那1帧的坐标
            s_framesV2[frameStart].leftX = lx;
            s_framesV2[frameStart].leftY = ly;
            s_framesV2[frameStart].rightX = rx;
            s_framesV2[frameStart].rightY = ry;
        }
    }
    
    // 更新动作的虚拟方向
    s_actions[actionIndex].stickL = leftDir;
    s_actions[actionIndex].stickR = rightDir;
    s_actions[actionIndex].stickMergedVirtual = false;
    s_actions[actionIndex].modified = true;
}

// 移动动作到指定位置
void MacroData::moveAction(s32 fromIndex, s32 toIndex) {
    if (fromIndex < 0 || fromIndex >= (s32)s_actions.size()) return;
    if (toIndex < 0 || toIndex > (s32)s_actions.size()) return;
    if (fromIndex == toIndex || fromIndex == toIndex - 1) return;
    
    // 保存快照
    saveSnapshot();
    
    // 计算 fromIndex 动作的帧起始位置和帧数
    s32 fromFrameStart = 0;
    for (s32 i = 0; i < fromIndex; i++) fromFrameStart += s_actions[i].frameCount;
    u32 frameCount = s_actions[fromIndex].frameCount;
    
    // 计算 toIndex 对应的帧插入位置
    s32 toFrameStart = 0;
    for (s32 i = 0; i < toIndex; i++) toFrameStart += s_actions[i].frameCount;
    
    // 移动帧数据
    if (s_header.version == 1) {
        std::vector<MacroFrame> extractedFrames(s_frames.begin() + fromFrameStart, s_frames.begin() + fromFrameStart + frameCount);
        s_frames.erase(s_frames.begin() + fromFrameStart, s_frames.begin() + fromFrameStart + frameCount);
        if (toFrameStart > fromFrameStart) toFrameStart -= frameCount;
        s_frames.insert(s_frames.begin() + toFrameStart, extractedFrames.begin(), extractedFrames.end());
    } else {
        std::vector<MacroFrameV2> extractedFrames( s_framesV2.begin() + fromFrameStart, s_framesV2.begin() + fromFrameStart + frameCount);
        s_framesV2.erase(s_framesV2.begin() + fromFrameStart, s_framesV2.begin() + fromFrameStart + frameCount);
        if (toFrameStart > fromFrameStart) toFrameStart -= frameCount;
        s_framesV2.insert(s_framesV2.begin() + toFrameStart, extractedFrames.begin(), extractedFrames.end());
    }
    
    // 标记受影响范围内的动作为 modified（toIndex 是插入位置，实际范围是 [from, to-1] 或 [to, from]）
    s32 rangeStart = (fromIndex < toIndex) ? fromIndex : toIndex;
    s32 rangeEnd = (fromIndex < toIndex) ? toIndex - 1 : fromIndex;
    for (s32 i = rangeStart; i <= rangeEnd && i < (s32)s_actions.size(); i++) {
        s_actions[i].modified = true;
    }
    
    // 移动动作
    Action extractedAction = s_actions[fromIndex];
    s_actions.erase(s_actions.begin() + fromIndex);
    if (toIndex > fromIndex) toIndex--;
    s_actions.insert(s_actions.begin() + toIndex, extractedAction);
}

// 修改持续时间（支持多选）
void MacroData::setActionDuration(s32 startIndex, s32 endIndex, u32 durationMs) {
    if (startIndex < 0 || endIndex >= (s32)s_actions.size() || startIndex > endIndex) return;
    saveSnapshot();
    
    if (s_header.version == 1) {
        // V1: 根据持续时间计算帧数，修改每个动作对应的帧数据
        u32 newFrameCount = (durationMs * s_header.frameRate + 500) / 1000;
        if (newFrameCount == 0) newFrameCount = 1;
        u32 actualDuration = newFrameCount * 1000 / s_header.frameRate;
        
        for (s32 i = startIndex; i <= endIndex; i++) {
            // 计算当前动作的帧起始位置
            s32 frameStart = 0;
            for (s32 j = 0; j < i; j++)
                frameStart += s_actions[j].frameCount;
            
            u32 oldFrameCount = s_actions[i].frameCount;
            if (newFrameCount != oldFrameCount) {
                MacroFrame templateFrame = s_frames[frameStart];
                if (newFrameCount > oldFrameCount)
                    s_frames.insert(s_frames.begin() + frameStart + oldFrameCount, newFrameCount - oldFrameCount, templateFrame);
                else
                    s_frames.erase(s_frames.begin() + frameStart + newFrameCount, s_frames.begin() + frameStart + oldFrameCount);
            }
            s_actions[i].frameCount = newFrameCount;
            s_actions[i].duration = actualDuration;
            s_actions[i].modified = true;
        }
        s_header.frameCount = s_frames.size();
    } else {
        // V2: 直接修改每个动作对应的帧的持续时间（每个动作都是1帧）
        for (s32 i = startIndex; i <= endIndex; i++) {
            // 计算当前动作的帧起始位置
            s32 frameStart = 0;
            for (s32 j = 0; j < i; j++)
                frameStart += s_actions[j].frameCount;
            
            s_framesV2[frameStart].durationMs = durationMs;
            s_actions[i].duration = durationMs;
            s_actions[i].modified = true;
        }
    }
    
    updateDurationMs();
}

// 修改动作的触发按钮
void MacroData::setActionButtons(s32 actionIndex, u64 buttons) {
    if (actionIndex < 0 || actionIndex >= (s32)s_actions.size()) return;
    saveSnapshot();
    
    // 更新动作的按键
    s_actions[actionIndex].buttons = buttons;
    s_actions[actionIndex].modified = true;
    
    // 计算帧起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < actionIndex; i++)
        frameStart += s_actions[i].frameCount;
    
    // 更新帧的按键
    if (s_header.version == 1) {
        // V1: 修改该动作对应的所有帧
        for (u32 i = 0; i < s_actions[actionIndex].frameCount; i++)
            s_frames[frameStart + i].keysHeld = buttons;
    } else {
        // V2: 直接修改那一帧
        s_framesV2[frameStart].keysHeld = buttons;
    }
}

// 保存快照，用于撤销
void MacroData::saveSnapshot() {
    if (s_undoStack.size() >= 3) s_undoStack.erase(s_undoStack.begin());
    s_undoStack.push_back({s_frames, s_framesV2, s_actions});
}

// 查询能否撤销
bool MacroData::canUndo() {
    return !s_undoStack.empty();
}

// 撤销
bool MacroData::undo() {
    if (s_undoStack.empty()) return false;
    auto& snapshot = s_undoStack.back();
    s_frames = snapshot.frames;
    s_framesV2 = snapshot.framesV2;
    s_actions = snapshot.actions;
    s_header.frameCount = (s_header.version == 1) ? s_frames.size() : s_framesV2.size();
    s_undoStack.pop_back();
    updateDurationMs();
    return true;
}

// 清理内存
void MacroData::allCleanup() {
    s_frames.clear();
    s_frames.shrink_to_fit();
    s_framesV2.clear();
    s_framesV2.shrink_to_fit();
    s_actions.clear();
    s_actions.shrink_to_fit();
    s_basicInfo = {};
    undoCleanup();
}

// 清理撤销快照内存
void MacroData::undoCleanup() {
    s_undoStack.clear();
    s_undoStack.shrink_to_fit();
}
