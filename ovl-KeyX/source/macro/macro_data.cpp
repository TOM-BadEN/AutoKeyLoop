#include "macro_data.hpp"
#include "macro_util.hpp"
#include <cstdio>
#include <sys/stat.h>
#include <cstring>

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
    s_frames.clear();
    s_framesV2.clear();
    s_basicInfo = {};
    FILE* fp = fopen(filePath, "rb");
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
    if (stat(filePath, &st) == 0) s_basicInfo.fileSize = st.st_size;
    else s_basicInfo.fileSize = 0;
    // 获取显示名称（优先从元数据读取，否则用文件名）
    std::string displayName = MacroUtil::getDisplayName(filePath);
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
    FILE* fp = fopen(s_filePath, "wb");
    if (!fp) return false;
    if (s_header.version == 1) saveForEditV1(fp);
    else saveForEditV2(fp);
    fclose(fp);
    return true;
}

// V1: 保存帧数据
void MacroData::saveForEditV1(FILE* fp) {
    if (!s_frames.empty()) {
        auto& f = s_frames.back();
        if (f.keysHeld | f.leftX | f.leftY | f.rightX | f.rightY) s_frames.insert(s_frames.end(), 2, {});
    }
    s_header.frameCount = s_frames.size();
    fwrite(&s_header, sizeof(MacroHeader), 1, fp);
    fwrite(s_frames.data(), sizeof(MacroFrame), s_frames.size(), fp);
}

// V2: 保存帧数据
void MacroData::saveForEditV2(FILE* fp) {
    if (!s_framesV2.empty()) {
        auto& f = s_framesV2.back();
        if (f.keysHeld | f.leftX | f.leftY | f.rightX | f.rightY) s_framesV2.push_back({33, 0, 0, 0, 0, 0});
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
    for (size_t i = 1; i < s_frames.size(); i++) {
        if (isSameState(s_frames[i], s_frames[i-1])) {
            current.frameCount++;
        } else {
            current.duration = current.frameCount * 1000 / s_header.frameRate;
            s_actions.push_back(current);
            current.buttons = s_frames[i].keysHeld;
            current.stickL = getStickDir(s_frames[i].leftX, s_frames[i].leftY);
            current.stickR = getStickDir(s_frames[i].rightX, s_frames[i].rightY);
            current.frameCount = 1;
        }
    }
    current.duration = current.frameCount * 1000 / s_header.frameRate;
    s_actions.push_back(current);
}

// V2: 解析帧数据到动作列表
void MacroData::parseActionsV2() {
    if (s_framesV2.empty()) return;
    Action current;
    current.buttons = s_framesV2[0].keysHeld;
    current.stickL = getStickDir(s_framesV2[0].leftX, s_framesV2[0].leftY);
    current.stickR = getStickDir(s_framesV2[0].rightX, s_framesV2[0].rightY);
    current.duration = s_framesV2[0].durationMs;
    current.frameCount = 1;
    for (size_t i = 1; i < s_framesV2.size(); i++) {
        if (isSameState(s_framesV2[i], s_framesV2[i-1])) {
            current.duration += s_framesV2[i].durationMs;
            current.frameCount++;
        } else {
            s_actions.push_back(current);
            current.buttons = s_framesV2[i].keysHeld;
            current.stickL = getStickDir(s_framesV2[i].leftX, s_framesV2[i].leftY);
            current.stickR = getStickDir(s_framesV2[i].rightX, s_framesV2[i].rightY);
            current.duration = s_framesV2[i].durationMs;
            current.frameCount = 1;
        }
    }
    s_actions.push_back(current);
}

// 获取所有动作
std::vector<Action>& MacroData::getActions() {
    return s_actions;
}

// 对比两个动作的数据是否相同
bool MacroData::isSameAction(const Action& a, const Action& b) {
    return (a.buttons == b.buttons) && 
           (a.stickL == b.stickL) && 
           (a.stickR == b.stickR);
}

// 尝试合并指定位置与前后相邻的相同动作
void MacroData::tryMergeAt(s32 actionIndex) {
    if (actionIndex < 0 || actionIndex >= (s32)s_actions.size()) return;
    
    // 先尝试与后面合并
    if (actionIndex + 1 < (s32)s_actions.size() && 
        isSameAction(s_actions[actionIndex], s_actions[actionIndex + 1])) {
        s_actions[actionIndex].duration += s_actions[actionIndex + 1].duration;
        s_actions[actionIndex].frameCount += s_actions[actionIndex + 1].frameCount;
        s_actions[actionIndex].modified = s_actions[actionIndex].modified || s_actions[actionIndex + 1].modified;
        s_actions.erase(s_actions.begin() + actionIndex + 1);
    }
    
    // 再尝试与前面合并
    if (actionIndex > 0 && 
        isSameAction(s_actions[actionIndex - 1], s_actions[actionIndex])) {
        s_actions[actionIndex - 1].duration += s_actions[actionIndex].duration;
        s_actions[actionIndex - 1].frameCount += s_actions[actionIndex].frameCount;
        s_actions[actionIndex - 1].modified = s_actions[actionIndex - 1].modified || s_actions[actionIndex].modified;
        s_actions.erase(s_actions.begin() + actionIndex);
    }
}

// 删除后尝试合并
void MacroData::tryMergeAfterDelete(s32 deletedIndex) {
    // 删除后，检查 deletedIndex-1 与 deletedIndex 位置是否可合并
    // （deletedIndex 现在是原来 deletedIndex+1 的位置）
    if (deletedIndex > 0 && deletedIndex < (s32)s_actions.size() &&
        isSameAction(s_actions[deletedIndex - 1], s_actions[deletedIndex])) {
        s_actions[deletedIndex - 1].duration += s_actions[deletedIndex].duration;
        s_actions[deletedIndex - 1].frameCount += s_actions[deletedIndex].frameCount;
        s_actions[deletedIndex - 1].modified = s_actions[deletedIndex - 1].modified || s_actions[deletedIndex].modified;
        s_actions.erase(s_actions.begin() + deletedIndex);
    }
}

// 插入一个新的动作（无动作，默认100ms）
void MacroData::insertAction(s32 actionIndex, bool insertBefore) {
    saveSnapshot();
    s32 insertPos = insertBefore ? actionIndex : actionIndex + 1;
    
    // 计算帧插入位置（V1/V2 统一用 frameCount）
    s32 frameInsertPos = 0;
    for (s32 i = 0; i < insertPos && i < (s32)s_actions.size(); i++)
        frameInsertPos += s_actions[i].frameCount;
    
    if (s_header.version == 1) {
        // V1: 计算100ms对应的帧数（四舍五入）
        u32 newFrameCount = (100 * s_header.frameRate + 500) / 1000;
        u32 durationMs = newFrameCount * 1000 / s_header.frameRate;
        Action newAction = {0, StickDir::None, StickDir::None, durationMs, newFrameCount, true};
        s_frames.insert(s_frames.begin() + frameInsertPos, newFrameCount, {});
        s_actions.insert(s_actions.begin() + insertPos, newAction);
        s_header.frameCount = s_frames.size();
    } else {
        // V2: 直接用100ms
        Action newAction = {0, StickDir::None, StickDir::None, 100, 1, true};
        s_framesV2.insert(s_framesV2.begin() + frameInsertPos, {100, 0, 0, 0, 0, 0});
        s_actions.insert(s_actions.begin() + insertPos, newAction);
        s_header.frameCount = s_framesV2.size();
    }
    
    tryMergeAt(insertPos);
}

void MacroData::resetActions(s32 startIndex, s32 endIndex) {
    saveSnapshot();
    
    // 计算总帧数和总时长
    u32 totalFrames = 0;
    u32 totalDuration = 0;
    for (s32 i = startIndex; i <= endIndex; i++) {
        totalFrames += s_actions[i].frameCount;
        totalDuration += s_actions[i].duration;
    }
    
    // 计算帧起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < startIndex; i++)
        frameStart += s_actions[i].frameCount;
    
    // 清零帧数据
    if (s_header.version == 1) {
        for (u32 i = 0; i < totalFrames; i++)
            s_frames[frameStart + i] = {};
    } else {
        // V2: 合并成1帧
        s_framesV2[frameStart] = {totalDuration, 0, 0, 0, 0, 0};
        if (totalFrames > 1) {
            s_framesV2.erase(s_framesV2.begin() + frameStart + 1, s_framesV2.begin() + frameStart + totalFrames);
            s_header.frameCount = s_framesV2.size();
        }
    }
    
    // 删除选中范围的动作
    for (s32 i = endIndex; i >= startIndex; i--)
        s_actions.erase(s_actions.begin() + i);
    
    // 插入新的无动作
    u32 newFrameCount = (s_header.version == 1) ? totalFrames : 1;
    Action newAction = {0, StickDir::None, StickDir::None, totalDuration, newFrameCount, true};
    s_actions.insert(s_actions.begin() + startIndex, newAction);
    
    tryMergeAt(startIndex);
}

void MacroData::deleteActions(s32 startIndex, s32 endIndex) {
    saveSnapshot();
    
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
    
    tryMergeAfterDelete(startIndex);
}

// 修改持续时间
void MacroData::setActionDuration(s32 actionIndex, u32 durationMs) {
    if (actionIndex < 0 || actionIndex >= (s32)s_actions.size()) return;
    saveSnapshot();
    
    // 计算帧起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < actionIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    if (s_header.version == 1) {
        // V1: 需要调整帧数
        u32 newFrameCount = (durationMs * s_header.frameRate + 500) / 1000;
        if (newFrameCount == 0) newFrameCount = 1;
        u32 oldFrameCount = s_actions[actionIndex].frameCount;
        if (newFrameCount != oldFrameCount) {
            MacroFrame templateFrame = s_frames[frameStart];
            if (newFrameCount > oldFrameCount) s_frames.insert(s_frames.begin() + frameStart + oldFrameCount, newFrameCount - oldFrameCount, templateFrame);
            else s_frames.erase(s_frames.begin() + frameStart + newFrameCount, s_frames.begin() + frameStart + oldFrameCount);
            s_header.frameCount = s_frames.size();
        }
        s_actions[actionIndex].frameCount = newFrameCount;
        s_actions[actionIndex].duration = newFrameCount * 1000 / s_header.frameRate;
    } else {
        // V2: 直接修改帧的 durationMs
        s_framesV2[frameStart].durationMs = durationMs;
        s_actions[actionIndex].duration = durationMs;
    }
    
    s_actions[actionIndex].modified = true;
    tryMergeAt(actionIndex);
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
    for (s32 i = 0; i < actionIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    // 更新帧的按键
    if (s_header.version == 1) for (u32 i = 0; i < s_actions[actionIndex].frameCount; i++) s_frames[frameStart + i].keysHeld = buttons;
    else s_framesV2[frameStart].keysHeld = buttons;
    
    tryMergeAt(actionIndex);
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
}

// 清理撤销快照内存
void MacroData::undoCleanup() {
    s_undoStack.clear();
    s_undoStack.shrink_to_fit();
}
