#include "macro_rename.hpp"
#include "macro_list.hpp"
#include "macro_view.hpp"
#include "macro_util.hpp"
#include <ultra.hpp>
#include <cstdio>
#include "refresh.hpp"
#include "ipc.hpp"

namespace {
    constexpr const char* KeyboardRowsUpper[] = {
        "1234567890",
        "QWERTYUIOP",
        "ASDFGHJKL_",
        "ZXCVBNM"
    };

    constexpr const char* KeyboardRowsLower[] = {
        "1234567890",
        "qwertyuiop",
        "asdfghjkl_",
        "zxcvbnm"
    };

    constexpr const int kRowCount = 4;        // 键盘行数
    constexpr const u8 kMaxInputLen = 20;     // 最大输入长度
    constexpr const u8 kRepeatFrames = 8;     // 重复帧数(用来控制按键的重复触发)
}

MacroRenameGui::MacroRenameGui(const char* macroFilePath, const char* gameName, bool isRecord)
 : m_isRecord(isRecord)
{
    strcpy(m_macroFilePath, macroFilePath);
    strcpy(m_gameName, gameName);
    std::string fileName = ult::getFileName(macroFilePath);
    auto dot = fileName.rfind('.');
    if (dot != std::string::npos) fileName = fileName.substr(0, dot); 
    strncpy(m_input, fileName.c_str(), sizeof(m_input) - 1);
    m_input[sizeof(m_input) - 1] = '\0';
    m_inputLen = strlen(m_input);
    strcpy(m_inputOld, m_input);
}

tsl::elm::Element* MacroRenameGui::createUI() {

    auto frame = new tsl::elm::OverlayFrame(m_gameName, m_isRecord ? "是否保存当前脚本" : "修改脚本文件的名字");

    auto keyboard = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        const char* const* keyboard = m_useLower ? KeyboardRowsLower : KeyboardRowsUpper;
        // 绘制输入框和输入文本
        s32 inputX = x - 15;
        s32 inputY = y + 15;
        s32 inputW = 405;
        r->drawRect(inputX, inputY, inputW, 50, tsl::Color(0x5, 0x5, 0x5, 0xF));
        auto inputDim = r->getTextDimensions(m_input, false, 22);
        s32 centerX = inputX + inputW / 2;
        s32 textX = centerX - inputDim.first / 2;
        tsl::Color inputColor = m_isDuplicate ? tsl::Color(0xF,0x3,0x3,0xF) : tsl::Color(0xF,0xF,0xF,0xF);
        r->drawString(m_input, false, textX, inputY + 35, 22, inputColor);
        
        // 光标
        if ((m_cursorBlink / 30) % 2 == 0) {
            r->drawRect(textX + inputDim.first + 2, inputY + 15, 2, 20, tsl::Color(0x0,0xD,0xF,0xF));
        }
        
        // 键盘
        s32 keyY = inputY + 80;
        const s32 keyW = 36;
        const s32 spacing = 5;

        for (u32 row = 0; row < kRowCount; ++row) {
            const char* chars = keyboard[row];
            u32 count = static_cast<u32>(strlen(chars));
            s32 startX = x - 15;
            for (u32 col = 0; col < count; ++col) {
                s32 kx = startX + static_cast<s32>(col) * (keyW + spacing);
                bool sel = (m_row == static_cast<int>(row) && m_col == static_cast<int>(col));
                r->drawRect(kx, keyY, keyW, 45, sel ? tsl::Color(0x3,0x8,0xA,0xF) : tsl::Color(0x5,0x5,0x5,0xF));
                char txt[2] = {chars[col], '\0'};
                auto txtDim = r->getTextDimensions(txt, false, 22);
                s32 textX = kx + (keyW - txtDim.first) / 2;
                r->drawString(txt, false, textX, keyY + 33, 22, tsl::Color(0xF, 0xF, 0xF, 0xF));
            }
            keyY += 55;
        }

        // 绘制字符长度图标
        char txtLen[8];
        snprintf(txtLen, sizeof(txtLen), "%lu / %u", m_inputLen, kMaxInputLen);
        s32 boxX = x - 15 + 7 * keyW + 7 * spacing;
        s32 boxWidth = keyW * 3 + 2 * spacing;
        auto txtDim = r->getTextDimensions(txtLen, false, 22);
        s32 text2X = boxX + (boxWidth - txtDim.first) / 2;
        bool overLen = (m_inputLen != kMaxInputLen);
        r->drawRect(boxX, keyY - 55, boxWidth, 45, tsl::Color(0x5,0x5,0x5,0xF));
        r->drawString(txtLen, false, text2X, keyY - 22, 22, overLen ? tsl::Color(0xF,0xF,0xF,0xF) : tsl::Color(0xF,0x5,0x5,0xF));

        // 绘制用户指南
        s32 GuideX = x - 15;
        s32 GuideY = 470;
        r->drawString(" 用户指南", false, GuideX, GuideY, 20, tsl::Color(0xF,0xF,0xF,0xF));
        r->drawString("  重置输入     切换大小写", false, GuideX + 25, GuideY + 35, 18, tsl::highlightColor2);
        if (!m_isRecord) r->drawString("  取消修改     保存修改", false, GuideX + 25, GuideY + 70, 18, tsl::highlightColor2);
        else r->drawString("  删除脚本     保存脚本", false, GuideX + 25, GuideY + 70, 18, tsl::Color(0xF,0x5,0x5,0xF));

    });

    frame->setContent(keyboard);
    return frame;
}


void MacroRenameGui::update() {
    m_cursorBlink++;   // 光标闪烁
}

bool MacroRenameGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {

    const char* const* keyboard = m_useLower ? KeyboardRowsLower : KeyboardRowsUpper;

    // ===========================方向键处理==============================

    // 获取键盘row行的所有字符数
    auto rowLength = [&](int row) { return static_cast<int>(strlen(keyboard[row])); };

    // 处理边界问题，当超过边界的时候-1锁定在末尾
    auto fixColumnRange = [&]() { if (m_col >= rowLength(m_row)) m_col = rowLength(m_row) - 1; };

    // 根据传入的方向按钮调整光标位置：上下时会重新 clamp 当前列，左右时只在范围内移动
    auto move = [&](HidNpadButton dir) {
        if (dir == HidNpadButton_AnyDown && m_row < kRowCount - 1) {
            ++m_row;
            fixColumnRange();
        } else if (dir == HidNpadButton_AnyUp && m_row > 0) {
            --m_row;
            fixColumnRange();
        } else if (dir == HidNpadButton_AnyLeft && m_col > 0) {
            --m_col;
        } else if (dir == HidNpadButton_AnyRight && m_col < rowLength(m_row) - 1) {
            ++m_col;
        }
    };

    // 统一处理方向键按下：移动光标、记录最后方向并重置连续触发计数
    auto handleDir = [&](HidNpadButton dir) {
        move(dir);
        m_lastDir = dir;
        m_dirTick = kRepeatFrames;
        return true;
    };
    
    
    // 单次按下方向键
    if (keysDown & HidNpadButton_AnyDown)  return handleDir(HidNpadButton_AnyDown);
    else if (keysDown & HidNpadButton_AnyUp)    return handleDir(HidNpadButton_AnyUp);
    else if (keysDown & HidNpadButton_AnyLeft)  return handleDir(HidNpadButton_AnyLeft);
    else if (keysDown & HidNpadButton_AnyRight) return handleDir(HidNpadButton_AnyRight);
    else if (m_lastDir && (keysHeld & m_lastDir)) { // 长按重复方向键
        if (--m_dirTick == 0) {
            move(m_lastDir);
            m_dirTick = kRepeatFrames;
        }
        return true;
    }

    // 重置
    m_lastDir = (HidNpadButton)0;
    m_dirTick = 0;

    // ===========================功能键处理==============================

    // 添加所选的字符
    auto appendChar = [&]() {
        if (m_inputLen == kMaxInputLen) return;           
        if (m_inputLen < sizeof(m_input) - 1) {
            m_input[m_inputLen++] = keyboard[m_row][m_col];
            m_input[m_inputLen] = '\0';
            checkDuplicate();
        }
    };

    // 删除字符
    auto tryDeleteChar = [&]() {
        if (m_inputLen == 0) return false;
        --m_inputLen;
        m_input[m_inputLen] = '\0';
        m_backspaceTick = kRepeatFrames;
        checkDuplicate();
        return true;
    };


    // 功能按键
    if (keysDown & HidNpadButton_Minus) {           // - 返回
        if (m_isRecord) ult::deleteFileOrDirectory(m_macroFilePath);
        tsl::goBack();
        return true;
    } else if (keysDown & HidNpadButton_Plus) {     // + 保存
        MacroRename();
        return true;
    } else if (keysDown & HidNpadButton_A) {        // 插件文字
        appendChar();
        return true;
    } else if (keysDown & HidNpadButton_B) {        // 单按 B
        tryDeleteChar();
        return true;
    } else if (keysHeld & HidNpadButton_B) {        // 长按 B
        if (m_backspaceTick > 0 && --m_backspaceTick > 0) return true;
        tryDeleteChar();
        return true;
    } else if (keysDown & HidNpadButton_Y) {         // 切换大小写
        m_useLower = !m_useLower;
        return true;
    } else if (keysDown & HidNpadButton_X) {         // 重置
        strcpy(m_input, m_inputOld);
        m_inputLen = strlen(m_input);
        checkDuplicate();
        return true;
    }

    // 重置
    m_backspaceTick = 0;

    // ===================没有返回true，完全接管按键================= 
    return true;
}

void MacroRenameGui::MacroRename(){
    if (m_inputLen == 0 || m_isDuplicate) return;

    // 获取目录长度：sdmc:/config/KeyX/macros/TID/old.macro -> dirLen = 到最后一个 '/' 的位置 + 1
    const char* lastSlash = strrchr(m_macroFilePath, '/');
    if (!lastSlash) return;
    size_t dirLen = lastSlash - m_macroFilePath + 1; 

    // 获取新路径：sdmc:/config/KeyX/macros/TID/ + m_input + .macro
    char newMacroFilePath[96];
    memcpy(newMacroFilePath, m_macroFilePath, dirLen);
    snprintf(newMacroFilePath + dirLen, sizeof(newMacroFilePath) - dirLen, "%s.macro", m_input);
    
    // 复制一个game名字
    char gameName[64];
    strcpy(gameName, m_gameName);

    // 文件改名（内部含元数据更新以及快捷键配置文件的名字修改）
    bool needReload = MacroUtil::renameMacro(m_macroFilePath, newMacroFilePath);

    // 如果是从录制状态跳转进来的，就不需要重新加载宏列表
    bool isRecord = m_isRecord;
    if (!isRecord) {  
        if (needReload) g_ipcManager.sendReloadMacroCommand();    
        Refresh::RefrRequest(Refresh::MacroGameList);  
        tsl::swapTo<MacroViewGui>(SwapDepth(2), newMacroFilePath, gameName, isRecord);
    } 
    else tsl::swapTo<MacroViewGui>(SwapDepth(1), newMacroFilePath, gameName, isRecord);
}

void MacroRenameGui::checkDuplicate() {
    // 构造新路径
    const char* lastSlash = strrchr(m_macroFilePath, '/');
    if (!lastSlash) { m_isDuplicate = false; return; }
    size_t dirLen = lastSlash - m_macroFilePath + 1;
    
    char newPath[96];
    memcpy(newPath, m_macroFilePath, dirLen);
    snprintf(newPath + dirLen, sizeof(newPath) - dirLen, "%s.macro", m_input);
    
    // 如果新路径等于原路径，不算重名
    if (strcmp(newPath, m_macroFilePath) == 0) {
        m_isDuplicate = false;
        return;
    }
    
    // 检查文件是否存在
    m_isDuplicate = ult::isFile(newPath);
}
