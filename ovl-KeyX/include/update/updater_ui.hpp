#pragma once
#include <tesla.hpp>
#include <atomic>
#include "updater_data.hpp"

enum class UpdateState {
    GettingJson,    // 正在获取更新信息
    NetworkError,   // 网络错误
    NoUpdate,       // 已是最新版本
    HasUpdate       // 有新版本可用
};

class UpdaterUI : public tsl::Gui 
{
public:
    UpdaterUI();
    ~UpdaterUI();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;

private:
    UpdateState m_state = UpdateState::GettingJson;                          // 当前状态
    UpdaterData m_updateData;                                                // 更新数据获取器
    UpdateInfo m_updateInfo;                                                 // 更新信息
    int m_frameIndex = 0;                                                    // 动画帧索引
    int m_frameCounter = 0;                                                  // 帧计数器

    void drawGettingJson(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h); // 绘制加载中
    void drawNetworkError(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);// 绘制网络错误
    void drawNoUpdate(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);    // 绘制无需更新
    void drawHasUpdate(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h);   // 绘制有新版本

    // 线程相关
    Thread m_thread{};                                                       // 线程对象
    bool m_threadCreated = false;                                            // 线程是否已创建
    std::atomic<bool> m_taskDone{false};                                     // 任务完成标志
    static char s_threadStack[32 * 1024];                                       // 线程栈（必须静态）
    static void ThreadFunc(void* arg);                                       // 线程函数（必须静态）
    void startThread();                                                      // 启动线程
    void stopThread();                                                       // 停止线程
};
