#pragma once
#include <tesla.hpp>
#include <vector>

// 宏文件头
struct MacroHeader {
    char magic[4];      // "KEYX"
    u16 version;        // 版本号
    u16 frameRate;      // 帧率
    u64 titleId;        // 游戏TID
    u32 frameCount;     // 总帧数
} __attribute__((packed));

// 宏单帧数据
struct MacroFrame {
    u64 keysHeld;       // 按键状态
    s32 leftX;          // 左摇杆X
    s32 leftY;          // 左摇杆Y
    s32 rightX;         // 右摇杆X
    s32 rightY;         // 右摇杆Y
} __attribute__((packed));

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
    RecordingFrame* m_frame;
    u64 m_lastUpdatedSeconds;
    u64 m_lastFocusCheckMs;  // 上次焦点检测的时间（毫秒）
    
    // 宏录制数据
    std::vector<MacroFrame> m_frames;  // 录制的帧数据
    
    void exitRecording();  // 退出录制并返回主界面
    void saveToFile();     // 保存录制数据到文件
};