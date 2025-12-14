#pragma once
#include <tesla.hpp>
#include <vector>
#include "macro_data.hpp"

// 录制用的 Frame
class RecordingFrame : public tsl::elm::Element
{
public:
    RecordingFrame();
    void setRecordingTime(u64 elapsed_s);
    virtual void draw(tsl::gfx::Renderer* renderer) override;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override;

private:
    char m_RecordingTimeText[32];
};

// 录制时界面类
class RecordingGui : public tsl::Gui 
{
public:
    RecordingGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    u64 m_startTime;
    RecordingFrame* m_frame = nullptr;
    u64 m_lastUpdatedSeconds = 0;           // 上次更新时间（秒）
    u64 m_lastFocusCheckMs = 0;             // 上次焦点检测的时间（毫秒）
    
    // 宏录制数据
    // std::vector<MacroFrame> m_frames;  // V1，与1.4.1版本弃用
    std::vector<MacroFrameV2> m_frames;  // V2
    u32 m_totalSamples = 0;              // 总采样次数（含合并的）
    u32 m_lastFrameMs = 0;               // 上一帧的时间（用于计算持续时间）
    
    void exitRecording();  // 退出录制并返回主界面
    void saveToFile();     // 保存录制数据到文件
};