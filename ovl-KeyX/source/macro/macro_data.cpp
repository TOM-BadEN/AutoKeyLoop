#include "macro_data.hpp"
#include <cstdio>
#include <sys/stat.h>
#include <cstring>

// 静态成员定义
char MacroData::s_filePath[128];
MacroHeader MacroData::s_header;
MacroBasicInfo MacroData::s_basicInfo;
std::vector<MacroFrame> MacroData::s_frames;
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

// 加载获取宏基础数据
bool MacroData::load(const char* filePath) {
    strncpy(s_filePath, filePath, sizeof(s_filePath) - 1);
    s_filePath[sizeof(s_filePath) - 1] = '\0';
    s_frames.clear();
    FILE* fp = fopen(filePath, "rb");
    if (!fp) return false;
    // 读取文件头
    fread(&s_header, sizeof(MacroHeader), 1, fp);
    // 获取所有帧数据
    s_frames.resize(s_header.frameCount);
    fread(s_frames.data(), sizeof(MacroFrame), s_header.frameCount, fp);
    fclose(fp);
    
    // 填充基础信息
    s_basicInfo.titleId = s_header.titleId;
    s_basicInfo.frameRate = s_header.frameRate;
    s_basicInfo.frameCount = s_header.frameCount;
    s_basicInfo.durationMs = s_header.frameRate ? (s_header.frameCount * 1000 / s_header.frameRate) : 0;
    // 文件大小
    struct stat st{};
    if (stat(filePath, &st) == 0) s_basicInfo.fileSize = st.st_size;
    else s_basicInfo.fileSize = 0;
    // 提取文件名
    const char* lastSlash = strrchr(filePath, '/');
    const char* name = lastSlash ? lastSlash + 1 : filePath;
    const char* dot = strrchr(name, '.');
    size_t len = dot ? (size_t)(dot - name) : strlen(name);
    if (len >= sizeof(s_basicInfo.fileName)) len = sizeof(s_basicInfo.fileName) - 1;
    strncpy(s_basicInfo.fileName, name, len);
    s_basicInfo.fileName[len] = '\0';
    
    return true;
}

// 获取文件路径
const char* MacroData::getFilePath() {
    return s_filePath;
}

// 保存编辑后的宏数据
bool MacroData::saveForEdit() {
    // 确保最后有释放帧（防止卡键）
    if (!s_frames.empty()) {
        auto& f = s_frames.back();
        if (f.keysHeld | f.leftX | f.leftY | f.rightX | f.rightY) s_frames.insert(s_frames.end(), 2, {});
    }

    FILE* fp = fopen(s_filePath, "wb");
    if (!fp) return false;
    s_header.frameCount = s_frames.size();
    fwrite(&s_header, sizeof(MacroHeader), 1, fp);
    // 写入所有帧
    fwrite(s_frames.data(), sizeof(MacroFrame), s_frames.size(), fp);
    fclose(fp);
    return true;
}

// 获取文件头数据
const MacroHeader& MacroData::getHeader() {
    return s_header;
}

// 获取基础信息
const MacroBasicInfo& MacroData::getBasicInfo() {
    return s_basicInfo;
}

// 解析所有帧数据的动作
void MacroData::parseActions() {
    s_actions.clear();
    if (s_frames.empty()) return;
    
    // 取每个动作的第一帧作为对比模板
    Action current;
    current.buttons = s_frames[0].keysHeld;
    current.stickL = getStickDir(s_frames[0].leftX, s_frames[0].leftY);
    current.stickR = getStickDir(s_frames[0].rightX, s_frames[0].rightY);
    current.frameCount = 1;
    // 遍历帧，合并相同状态
    for (size_t i = 1; i < s_frames.size(); i++) {
        if (isSameState(s_frames[i], s_frames[i-1])) {
            current.frameCount++;
        } else {
            // 如果当前动作与前一个动作不同，更新模板
            s_actions.push_back(current);
            current.buttons = s_frames[i].keysHeld;
            current.stickL = getStickDir(s_frames[i].leftX, s_frames[i].leftY);
            current.stickR = getStickDir(s_frames[i].rightX, s_frames[i].rightY);
            current.frameCount = 1;
        }
    }
    s_actions.push_back(current);  // 最后一个动作
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
        // 合并：将后面的帧数加到当前，删除后面的动作
        s_actions[actionIndex].frameCount += s_actions[actionIndex + 1].frameCount;
        s_actions[actionIndex].modified = s_actions[actionIndex].modified || s_actions[actionIndex + 1].modified;
        s_actions.erase(s_actions.begin() + actionIndex + 1);
    }
    
    // 再尝试与前面合并
    if (actionIndex > 0 && 
        isSameAction(s_actions[actionIndex - 1], s_actions[actionIndex])) {
        // 合并：将当前的帧数加到前面，删除当前动作
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
        s_actions[deletedIndex - 1].frameCount += s_actions[deletedIndex].frameCount;
        s_actions[deletedIndex - 1].modified = s_actions[deletedIndex - 1].modified || s_actions[deletedIndex].modified;
        s_actions.erase(s_actions.begin() + deletedIndex);
    }
}

// 插入一个新的动作（无动作，默认100ms）
void MacroData::insertAction(s32 actionIndex, bool insertBefore) {
    // 保存快照
    saveSnapshot();
    
    // 计算100ms对应的帧数
    u32 newFrameCount = s_header.frameRate / 10;  // 100ms = frameRate * 0.1
    if (newFrameCount == 0) newFrameCount = 1;
    
    // 创建新的 Action（无动作，标记为已修改）
    Action newAction = {0, StickDir::None, StickDir::None, newFrameCount, true};
    
    // 计算在 s_actions 中的插入位置
    s32 insertPos = insertBefore ? actionIndex : actionIndex + 1;
    
    // 计算在 s_frames 中的插入位置
    s32 frameInsertPos = 0;
    for (s32 i = 0; i < insertPos && i < (s32)s_actions.size(); i++) {
        frameInsertPos += s_actions[i].frameCount;
    }
    
    // 插入空帧到 s_frames
    MacroFrame emptyFrame = {0, 0, 0, 0, 0};
    s_frames.insert(s_frames.begin() + frameInsertPos, newFrameCount, emptyFrame);
    
    // 插入新动作到 s_actions
    s_actions.insert(s_actions.begin() + insertPos, newAction);
    
    // 尝试与相邻动作合并
    tryMergeAt(insertPos);
    
    // 更新 header.frameCount
    s_header.frameCount = s_frames.size();
}

void MacroData::resetActions(s32 startIndex, s32 endIndex) {
    saveSnapshot();
    
    // 计算总帧数
    u32 totalFrames = 0;
    for (s32 i = startIndex; i <= endIndex; i++) {
        totalFrames += s_actions[i].frameCount;
    }
    
    // 计算在 s_frames 中的起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < startIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    // 将 s_frames 中对应的帧全部清零
    MacroFrame emptyFrame = {0, 0, 0, 0, 0};
    for (u32 i = 0; i < totalFrames; i++) {
        s_frames[frameStart + i] = emptyFrame;
    }
    
    // 删除选中范围的动作（从后往前删）
    for (s32 i = endIndex; i >= startIndex; i--) {
        s_actions.erase(s_actions.begin() + i);
    }
    
    // 插入一个新的无动作（标记为已修改）
    Action newAction = {0, StickDir::None, StickDir::None, totalFrames, true};
    s_actions.insert(s_actions.begin() + startIndex, newAction);
    
    // 尝试与相邻动作合并
    tryMergeAt(startIndex);
}

void MacroData::deleteActions(s32 startIndex, s32 endIndex) {
    saveSnapshot();
    
    // 计算在 s_frames 中的起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < startIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    // 计算要删除的总帧数
    u32 totalFrames = 0;
    for (s32 i = startIndex; i <= endIndex; i++) {
        totalFrames += s_actions[i].frameCount;
    }
    
    // 从 s_frames 中删除对应的帧
    s_frames.erase(s_frames.begin() + frameStart, 
                   s_frames.begin() + frameStart + totalFrames);
    
    // 删除选中范围的动作
    s_actions.erase(s_actions.begin() + startIndex,
                    s_actions.begin() + endIndex + 1);
    
    // 更新 header.frameCount
    s_header.frameCount = s_frames.size();
    
    // 尝试合并删除位置前后的动作
    tryMergeAfterDelete(startIndex);
}

// 修改持续时间
void MacroData::setActionDuration(s32 actionIndex, u32 durationMs) {
    if (actionIndex < 0 || actionIndex >= (s32)s_actions.size()) return;
    saveSnapshot();
    
    // 计算新的帧数
    u32 newFrameCount = durationMs * s_header.frameRate / 1000;
    if (newFrameCount == 0) newFrameCount = 1;
    
    u32 oldFrameCount = s_actions[actionIndex].frameCount;
    if (newFrameCount == oldFrameCount) return;
    
    // 计算在 s_frames 中的起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < actionIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    // 获取该动作的帧模板
    MacroFrame templateFrame = s_frames[frameStart];
    
    if (newFrameCount > oldFrameCount) {
        // 增加帧：在末尾插入
        u32 addCount = newFrameCount - oldFrameCount;
        s_frames.insert(s_frames.begin() + frameStart + oldFrameCount, addCount, templateFrame);
    } else {
        // 减少帧：从末尾删除
        s_frames.erase(s_frames.begin() + frameStart + newFrameCount,
                       s_frames.begin() + frameStart + oldFrameCount);
    }
    
    // 更新动作的帧数
    s_actions[actionIndex].frameCount = newFrameCount;
    s_actions[actionIndex].modified = true;
    
    // 更新 header.frameCount
    s_header.frameCount = s_frames.size();
    
    // 尝试与相邻动作合并
    tryMergeAt(actionIndex);
}

// 修改动作的触发按钮
void MacroData::setActionButtons(s32 actionIndex, u64 buttons) {
    if (actionIndex < 0 || actionIndex >= (s32)s_actions.size()) return;
    saveSnapshot();
    
    // 更新动作的按键
    s_actions[actionIndex].buttons = buttons;
    s_actions[actionIndex].modified = true;
    
    // 计算在 s_frames 中的起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < actionIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    // 更新所有帧的按键
    for (u32 i = 0; i < s_actions[actionIndex].frameCount; i++) {
        s_frames[frameStart + i].keysHeld = buttons;
    }
    
    // 尝试与相邻动作合并
    tryMergeAt(actionIndex);
}

// 保存快照，用于撤销
void MacroData::saveSnapshot() {
    if (s_undoStack.size() >= 3) s_undoStack.erase(s_undoStack.begin());  // 删除最旧的
    s_undoStack.push_back({s_frames, s_actions});
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
    s_actions = snapshot.actions;
    s_header.frameCount = s_frames.size();
    s_undoStack.pop_back();
    return true;
}

// 清理内存
void MacroData::allCleanup() {
    s_frames.clear();
    s_frames.shrink_to_fit();
    s_actions.clear();
    s_actions.shrink_to_fit();
    s_undoStack.clear();
    s_undoStack.shrink_to_fit();
}

// 清理撤销快照内存
void MacroData::undoCleanup() {
    s_undoStack.clear();
    s_undoStack.shrink_to_fit();
}
