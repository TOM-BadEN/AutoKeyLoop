#include "macro_record.hpp"
#include "macro_setting.hpp"
#include "focus.hpp"
#include "game.hpp"
#include <ultra.hpp>
#include <time.h>

namespace {
    constexpr u64 STICK_PSEUDO_MASK = 0xFF0000ULL;  // 伪按键，必须过滤
}

// 录制用的 Frame
RecordingFrame::RecordingFrame() 
 : m_RecordingTimeText("REC  00:00")
{
}

void RecordingFrame::setRecordingTime(u64 elapsed_s) {
    snprintf(m_RecordingTimeText, sizeof(m_RecordingTimeText), "REC  %02lu:%02lu", elapsed_s / 60, elapsed_s % 60);
}

void RecordingFrame::draw(tsl::gfx::Renderer* renderer) {
    // 绘制全透明背景
    renderer->fillScreen(tsl::Color(0x0, 0x0, 0x0, 0x0));
    auto FrameWidthSize = tsl::cfg::FramebufferWidth - 60;
    auto FrameHightSize = 100;
    auto FrameX = 40;
    auto FrameY = 20;
    auto circleRadius = 12;
    auto circleX = FrameX + 25 + circleRadius;
    auto circleY = FrameY + FrameHightSize / 2;
    auto textFontSize = 50;
    auto textDim = renderer->getTextDimensions(m_RecordingTimeText, false, textFontSize);
    auto textX = circleX + circleRadius + 29;
    auto textY = (FrameY + FrameHightSize / 2 + textDim.second / 2) - 5;
    renderer->drawRoundedRect(FrameX, FrameY, FrameWidthSize, FrameHightSize, 10, tsl::Color(0x0, 0x0, 0x0, 0x8));
    renderer->drawStringWithColoredSections(
        m_RecordingTimeText,                // "REC  00:00"
        false,                              // 不使用等宽字体
        {"REC"},                            // 特殊符号列表
        textX, textY,                       // 坐标
        textFontSize,                       // 字体大小50
        tsl::Color(0xF, 0xF, 0xF, 0xF),     // 白色 - 默认颜色（用于时间部分"00:00"）
        tsl::Color(0xF, 0x5, 0x5, 0xF)      // 亮红色 - 特殊颜色（用于"REC"，和圆球同色）
    );
    renderer->drawCircle(circleX, circleY, circleRadius, true, tsl::Color(0xF, 0x5, 0x5, 0xF));
}

void RecordingFrame::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {
    this->setBoundaries(0, 0, tsl::cfg::FramebufferWidth, tsl::cfg::FramebufferHeight);
}

// 录制时界面类
RecordingGui::RecordingGui()
 : m_startTime(armGetSystemTick())
 , m_frame(nullptr)
 , m_lastUpdatedSeconds(0)
 , m_lastFocusCheckMs(0)  // 录制期间立即开始检查
{
    // 移动到屏幕中间
    u16 centerX = (tsl::cfg::ScreenWidth - tsl::cfg::LayerWidth);
    tsl::gfx::Renderer::get().setLayerPos(centerX, 0);
    tsl::hlp::requestForeground(false);
}

tsl::elm::Element* RecordingGui::createUI() {
    m_frame = new RecordingFrame();
    return m_frame;
}

// 保存录制数据到文件
void RecordingGui::saveToFile() {
    if (m_frames.empty()) return;
    // 构造文件头
    MacroHeader header;
    memcpy(header.magic, "KEYX", 4);
    header.version = 1;
    header.frameRate = 60;
    header.titleId = GameMonitor::getCurrentTitleId();
    header.frameCount = m_frames.size();
    // 生成文件路径
    char dirPath[64];
    sprintf(dirPath, "/config/KeyX/macros/%016lX", header.titleId);
    ult::createDirectory(dirPath);
    char filename[96];
    time_t timestamp = time(NULL);
    // 尝试基础文件名 
    // 如果文件已存在，添加后缀 -1, -2, -3...
    sprintf(filename, "%s/macro_%lu.macro", dirPath, (unsigned long)timestamp);
    int suffix = 1;
    while (ult::isFile(filename)) {
        sprintf(filename, "%s/macro_%lu-%d.macro", dirPath, (unsigned long)timestamp, suffix);
        suffix++;
    }
    // 写入文件
    FILE* fp = fopen(filename, "wb");
    if (fp) {
        fwrite(&header, sizeof(header), 1, fp);
        fwrite(m_frames.data(), sizeof(MacroFrame), m_frames.size(), fp);
        fclose(fp);
    }
}

// 退出录制并返回主界面
void RecordingGui::exitRecording() {
    tsl::gfx::Renderer::get().setLayerPos(0, 0);
    tsl::hlp::requestForeground(true);
    tsl::goBack();
    tsl::goBack();
    tsl::goBack();
}

void RecordingGui::update() {
    
    if (ult::currentForeground.load(std::memory_order_acquire)) tsl::hlp::requestForeground(false);
    
    u64 elapsed_ms = armTicksToNs(armGetSystemTick() - m_startTime) / 1000000ULL;
    
    // 每100ms检查一次焦点
    if (elapsed_ms >= m_lastFocusCheckMs + 100) {
        m_lastFocusCheckMs = elapsed_ms;
        FocusState focusState = FocusMonitor::GetState(GameMonitor::getCurrentTitleId());
        if (focusState == FocusState::OutOfFocus) {
            setRecordingMessage("录制已中断");
            exitRecording();
            return;
        }
    }
    
    // 更新显示（每1000ms更新一次）
    if (elapsed_ms >= m_lastUpdatedSeconds + 1000) {
        m_lastUpdatedSeconds = elapsed_ms;
        m_frame->setRecordingTime(elapsed_ms / 1000);
    }
    
    // 超时检测
    if (elapsed_ms > 30000) {
        setRecordingMessage("录制超时");
        exitRecording();
    }
}

bool RecordingGui::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    // 检测特斯拉快捷键
    const u64 combo = tsl::cfg::launchCombo;
    if (((keysHeld & combo) == combo) && (keysDown & combo)) {
        while (!m_frames.empty() && (m_frames.back().keysHeld & combo)) {
            m_frames.pop_back();
        }
        saveToFile();  // 保存文件
        setRecordingMessage("录制完成");
        exitRecording();
        return true;
    }
    
    // 录制当前帧数据
    MacroFrame frame;
    // 过滤摇杆虚拟按键（bit 16-23），只保留物理按键
    frame.keysHeld = keysHeld & ~STICK_PSEUDO_MASK;
    frame.leftX = joyStickPosLeft.x;
    frame.leftY = joyStickPosLeft.y;
    frame.rightX = joyStickPosRight.x;
    frame.rightY = joyStickPosRight.y;
    m_frames.push_back(frame);
    
    // 录制期间忽略所有其他按键
    return true;
}
