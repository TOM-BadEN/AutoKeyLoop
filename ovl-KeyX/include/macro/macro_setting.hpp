#pragma once
#include <tesla.hpp>

class SettingMacro : public tsl::Gui
{
public:
    SettingMacro();
    virtual tsl::elm::Element* createUI() override;
    
private:
    bool HandleRecordClick();
};

// 倒计时用的 Frame
class CountdownFrame : public tsl::elm::Element
{
public:
    CountdownFrame();
    void setCountdown(const char* text);
    virtual void draw(tsl::gfx::Renderer* renderer) override;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override;

private:
    char m_countdown2[4];
};

// 倒计时界面类
class CountdownGui : public tsl::Gui
{
public:
    CountdownGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;

private:
    u64 m_startTime;
    CountdownFrame* m_frame = nullptr;
    char m_countdown[4];
    u64 m_lastFocusCheckMs = 100;  // 上次焦点检测的时间（毫秒）
};

// 中间层用的 Frame
class interlayerFrame : public tsl::elm::Element
{
public:
    interlayerFrame();
    virtual void draw(tsl::gfx::Renderer* renderer) override;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override;

private:

};

// 中间层界面类
class interlayerGui : public tsl::Gui
{
public:
    interlayerGui();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;

private:

};


