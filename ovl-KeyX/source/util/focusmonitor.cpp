#include "focusmonitor.hpp"

// 辅助函数实现：从事件数组中查找指定tid的最后一个焦点状态
FocusState FocusMonitor::FindLastFocusState(const PdmAppletEvent* events, s32 total_out, u64 tid) {
    for (s32 i = total_out - 1; i >= 0; i--) {
        if (events[i].program_id != tid) continue;
        
        if (events[i].event_type == PdmAppletEventType_InFocus) {
            return FocusState::InFocus;
        } 
        else if (events[i].event_type == PdmAppletEventType_OutOfFocus || 
                 events[i].event_type == PdmAppletEventType_OutOfFocus4) {
            return FocusState::OutOfFocus;
        }
        break;
    }
    return FocusState::OutOfFocus;  // 找不到焦点事件，默认后台
}

// 静态成员初始化
u64 FocusMonitor::m_LastTid = 0;
s32 FocusMonitor::m_LastCheckedIndex = 0;
FocusState FocusMonitor::m_CurrentState = FocusState::OutOfFocus;

// 重置状态（游戏切换时）
void FocusMonitor::ResetForNewGame() {
    m_CurrentState = FocusState::OutOfFocus;
    
    // 获取事件范围
    s32 total = 0, start_index = 0, end_index = 0;
    Result rc = pdmqryGetAvailablePlayEventRange(&total, &start_index, &end_index);
    if (R_FAILED(rc)) {
        m_LastCheckedIndex = 0;
        return;
    }
    
    // 查询最近10个事件来初始化状态
    PdmAppletEvent events[10];
    s32 total_out = 0;
    s32 query_start = (end_index >= 9) ? (end_index - 9) : 0;
    
    rc = pdmqryQueryAppletEvent(query_start, false, events, 10, &total_out);
    
    if (R_SUCCEEDED(rc) && total_out > 0) {
        // 使用辅助函数查找最后一个焦点状态
        m_CurrentState = FindLastFocusState(events, total_out, m_LastTid);
    }
    
    m_LastCheckedIndex = end_index;
}

// 获取焦点状态（自动更新）
FocusState FocusMonitor::GetState(u64 tid) {
    if (tid == 0) return FocusState::OutOfFocus;

    // 游戏切换了，重置状态
    if (tid != m_LastTid) {
        m_LastTid = tid;
        ResetForNewGame();
        return m_CurrentState;  // 返回初始化后的状态
    }
    
    // 获取事件范围
    s32 total = 0, start_index = 0, end_index = 0;
    Result rc = pdmqryGetAvailablePlayEventRange(&total, &start_index, &end_index);
    
    if (R_FAILED(rc) || end_index <= m_LastCheckedIndex) {
        return m_CurrentState;  // 查询失败或无新事件，返回当前状态
    }
    
    // 查询新事件
    PdmAppletEvent events[10];
    s32 total_out = 0;
    
    rc = pdmqryQueryAppletEvent(m_LastCheckedIndex + 1, false, events, 10, &total_out);
    
    if (R_FAILED(rc) || total_out == 0) {
        m_LastCheckedIndex = end_index;
        return FocusState::OutOfFocus;  // 查询失败，返回后台状态
    }
    
    // 使用辅助函数查找最后一个焦点状态
    FocusState new_state = FindLastFocusState(events, total_out, tid);
    
    // 更新状态
    if (new_state != m_CurrentState) {
        m_CurrentState = new_state;
    }
    
    m_LastCheckedIndex = end_index;
    return m_CurrentState;  // 返回当前状态
}

