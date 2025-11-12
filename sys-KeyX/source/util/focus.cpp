#include "focus.hpp"

// 静态成员初始化
u64 FocusMonitor::m_LastTid = 0;
s32 FocusMonitor::m_LastCheckedIndex = 0;
FocusState FocusMonitor::m_CurrentState = FocusState::Unknown;

// 重置状态（游戏切换时）
void FocusMonitor::ResetForNewGame() {
    m_CurrentState = FocusState::Unknown;
    
    // 跳过历史事件
    s32 total = 0, start_index = 0, end_index = 0;
    Result rc = pdmqryGetAvailablePlayEventRange(&total, &start_index, &end_index);
    if (R_SUCCEEDED(rc)) {
        m_LastCheckedIndex = end_index;
    } else {
        m_LastCheckedIndex = 0;
    }
}

// 获取焦点状态（自动更新）
FocusState FocusMonitor::GetState(u64 tid) {
    // 游戏切换了，重置状态
    if (tid != m_LastTid) {
        m_LastTid = tid;
        ResetForNewGame();
        return FocusState::Unknown;
    }
    
    // 获取事件范围
    s32 total = 0, start_index = 0, end_index = 0;
    Result rc = pdmqryGetAvailablePlayEventRange(&total, &start_index, &end_index);
    
    if (R_FAILED(rc) || end_index <= m_LastCheckedIndex) {
        return FocusState::Unknown;  // 查询失败或无新事件
    }
    
    // 查询新事件
    PdmAppletEvent events[10];
    s32 total_out = 0;
    
    rc = pdmqryQueryAppletEvent(m_LastCheckedIndex + 1, false, events, 10, &total_out);
    
    if (R_FAILED(rc) || total_out == 0) {
        m_LastCheckedIndex = end_index;
        return FocusState::Unknown;  // 查询失败或无事件
    }
    
    // 从后往前查找最后一个匹配当前游戏的事件
    for (s32 i = total_out - 1; i >= 0; i--) {
        if (events[i].program_id != tid) continue;
        
        // 判断事件类型
        FocusState new_state = FocusState::Unknown;
        
        if (events[i].event_type == PdmAppletEventType_InFocus) {
            new_state = FocusState::InFocus;
        } 
        else if (events[i].event_type == PdmAppletEventType_OutOfFocus || 
                 events[i].event_type == PdmAppletEventType_OutOfFocus4) {
            new_state = FocusState::OutOfFocus;
        }
        
        // 更新状态
        if (new_state != FocusState::Unknown && new_state != m_CurrentState) {
            m_CurrentState = new_state;
            m_LastCheckedIndex = end_index;
            return new_state;
        }
        
        break;  // 只处理最后一个匹配的事件
    }
    
    m_LastCheckedIndex = end_index;
    return FocusState::Unknown;  // 状态未变化
}

