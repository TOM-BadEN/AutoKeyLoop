#include "macro_data.hpp"
#include <cstdio>

// 静态成员定义
MacroHeader MacroData::s_header;
std::vector<MacroFrame> MacroData::s_frames;
std::vector<Action> MacroData::s_actions;

// 撤销快照栈
std::vector<MacroData::UndoSnapshot> MacroData::s_undoStack;

// 摇杆死区 (约5%)
constexpr s32 STICK_DEADZONE = 1638;

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

bool MacroData::isSameState(const MacroFrame& a, const MacroFrame& b) {
    if (a.keysHeld != b.keysHeld) return false;
    if (getStickDir(a.leftX, a.leftY) != getStickDir(b.leftX, b.leftY)) return false;
    if (getStickDir(a.rightX, a.rightY) != getStickDir(b.rightX, b.rightY)) return false;
    return true;
}

bool MacroData::load(const char* filePath) {
    s_frames.clear();
    s_actions.clear();
    s_undoStack.clear();
    
    // 1. 打开文件
    FILE* fp = fopen(filePath, "rb");
    if (!fp) return false;
    
    // 2. 读取文件头
    fread(&s_header, sizeof(MacroHeader), 1, fp);
    
    // 3. 读取所有帧
    s_frames.resize(s_header.frameCount);
    fread(s_frames.data(), sizeof(MacroFrame), s_header.frameCount, fp);
    fclose(fp);
    
    return true;
}

bool MacroData::save(const char* filePath) {
    FILE* fp = fopen(filePath, "wb");
    if (!fp) return false;
    
    // 更新帧数
    s_header.frameCount = s_frames.size();
    
    // 写入文件头
    fwrite(&s_header, sizeof(MacroHeader), 1, fp);
    
    // 写入所有帧
    fwrite(s_frames.data(), sizeof(MacroFrame), s_frames.size(), fp);
    fclose(fp);
    
    return true;
}

const MacroHeader& MacroData::getHeader() {
    return s_header;
}

void MacroData::parseActions() {
    s_actions.clear();
    if (s_frames.empty()) return;
    
    // 遍历帧，合并相同状态
    Action current;
    current.buttons = s_frames[0].keysHeld;
    current.stickL = getStickDir(s_frames[0].leftX, s_frames[0].leftY);
    current.stickR = getStickDir(s_frames[0].rightX, s_frames[0].rightY);
    current.frameCount = 1;
    
    for (size_t i = 1; i < s_frames.size(); i++) {
        if (isSameState(s_frames[i], s_frames[i-1])) {
            current.frameCount++;
        } else {
            s_actions.push_back(current);
            current.buttons = s_frames[i].keysHeld;
            current.stickL = getStickDir(s_frames[i].leftX, s_frames[i].leftY);
            current.stickR = getStickDir(s_frames[i].rightX, s_frames[i].rightY);
            current.frameCount = 1;
        }
    }
    s_actions.push_back(current);  // 最后一个动作
}

std::vector<Action>& MacroData::getActions() {
    return s_actions;
}

bool MacroData::isSameAction(const Action& a, const Action& b) {
    return (a.buttons == b.buttons) && 
           (a.stickL == b.stickL) && 
           (a.stickR == b.stickR);
}

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

void MacroData::insertAction(s32 actionIndex, bool insertBefore) {
    saveSnapshot();
    
    // 1. 计算100ms对应的帧数
    u32 newFrameCount = s_header.frameRate / 10;  // 100ms = frameRate * 0.1
    if (newFrameCount == 0) newFrameCount = 1;
    
    // 2. 创建新的 Action（无动作，标记为已修改）
    Action newAction = {0, StickDir::None, StickDir::None, newFrameCount, true};
    
    // 3. 计算在 s_actions 中的插入位置
    s32 insertPos = insertBefore ? actionIndex : actionIndex + 1;
    
    // 4. 计算在 s_frames 中的插入位置
    s32 frameInsertPos = 0;
    for (s32 i = 0; i < insertPos && i < (s32)s_actions.size(); i++) {
        frameInsertPos += s_actions[i].frameCount;
    }
    
    // 5. 插入空帧到 s_frames
    MacroFrame emptyFrame = {0, 0, 0, 0, 0};
    s_frames.insert(s_frames.begin() + frameInsertPos, newFrameCount, emptyFrame);
    
    // 6. 插入新动作到 s_actions
    s_actions.insert(s_actions.begin() + insertPos, newAction);
    
    // 7. 尝试与相邻动作合并
    tryMergeAt(insertPos);
    
    // 8. 更新 header.frameCount
    s_header.frameCount = s_frames.size();
}

void MacroData::resetActions(s32 startIndex, s32 endIndex) {
    saveSnapshot();
    
    // 1. 计算总帧数
    u32 totalFrames = 0;
    for (s32 i = startIndex; i <= endIndex; i++) {
        totalFrames += s_actions[i].frameCount;
    }
    
    // 2. 计算在 s_frames 中的起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < startIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    // 3. 将 s_frames 中对应的帧全部清零
    MacroFrame emptyFrame = {0, 0, 0, 0, 0};
    for (u32 i = 0; i < totalFrames; i++) {
        s_frames[frameStart + i] = emptyFrame;
    }
    
    // 4. 删除选中范围的动作（从后往前删）
    for (s32 i = endIndex; i >= startIndex; i--) {
        s_actions.erase(s_actions.begin() + i);
    }
    
    // 5. 插入一个新的无动作（标记为已修改）
    Action newAction = {0, StickDir::None, StickDir::None, totalFrames, true};
    s_actions.insert(s_actions.begin() + startIndex, newAction);
    
    // 6. 尝试与相邻动作合并
    tryMergeAt(startIndex);
}

void MacroData::deleteActions(s32 startIndex, s32 endIndex) {
    saveSnapshot();
    
    // 1. 计算在 s_frames 中的起始位置
    s32 frameStart = 0;
    for (s32 i = 0; i < startIndex; i++) {
        frameStart += s_actions[i].frameCount;
    }
    
    // 2. 计算要删除的总帧数
    u32 totalFrames = 0;
    for (s32 i = startIndex; i <= endIndex; i++) {
        totalFrames += s_actions[i].frameCount;
    }
    
    // 3. 从 s_frames 中删除对应的帧
    s_frames.erase(s_frames.begin() + frameStart, 
                   s_frames.begin() + frameStart + totalFrames);
    
    // 4. 删除选中范围的动作
    s_actions.erase(s_actions.begin() + startIndex,
                    s_actions.begin() + endIndex + 1);
    
    // 5. 更新 header.frameCount
    s_header.frameCount = s_frames.size();
    
    // 6. 尝试合并删除位置前后的动作
    tryMergeAfterDelete(startIndex);
}

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
        u32 removeCount = oldFrameCount - newFrameCount;
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

void MacroData::saveSnapshot() {
    if (s_undoStack.size() >= 3) {
        s_undoStack.erase(s_undoStack.begin());  // 删除最旧的
    }
    s_undoStack.push_back({s_frames, s_actions});
}

bool MacroData::canUndo() {
    return !s_undoStack.empty();
}

bool MacroData::undo() {
    if (s_undoStack.empty()) return false;
    
    auto& snapshot = s_undoStack.back();
    s_frames = snapshot.frames;
    s_actions = snapshot.actions;
    s_header.frameCount = s_frames.size();
    s_undoStack.pop_back();
    return true;
}

void MacroData::cleanup() {
    s_frames.clear();
    s_frames.shrink_to_fit();
    s_actions.clear();
    s_actions.shrink_to_fit();
    s_undoStack.clear();
    s_undoStack.shrink_to_fit();
}
