#include "ToolStateMachine.h"
#include <wx/datetime.h>
#include <wx/utils.h>
#include <wx/image.h>
#include <wx/cursor.h>
#include "CanvasPanel.h"

// 事件定义
wxDEFINE_EVENT(TOOL_STATE_CHANGED, wxCommandEvent);

ToolStateMachine::ToolStateMachine(CanvasPanel* canvas)
	: m_canvas(canvas),
    m_currentTool(ToolType::DRAG_TOOL),
    m_dragState(DragToolState::IDLE),
    m_selectState(SelectToolState::IDLE),
    m_textState(TextToolState::IDLE),
    m_componentState(ComponentToolState::IDLE),
    m_wireState(WireToolState::IDLE),
    m_drawingState(DrawingToolState::IDLE) {

    // 初始化历史记录
    PushStateToHistory();
}

ToolStateMachine::~ToolStateMachine() {
}

void ToolStateMachine::SetCurrentTool(ToolType tool) {
    if (m_currentTool != tool) {
        // 保存当前状态到历史
        PushStateToHistory();

        ToolType oldTool = m_currentTool;
        m_currentTool = tool;

        // 重置新工具的状态（根据需求决定是否重置）
        ResetCurrentToolState();

        SendStateChangeEvent(wxString::Format("Tool changed from %d to %d",
            static_cast<int>(oldTool),
            static_cast<int>(tool)));
    }

    if (m_canvas) {
        switch (tool) {
        case ToolType::COMPONENT_TOOL: {
            m_canvas->SetCursor(wxCursor(wxCURSOR_CROSS));
            break;
        }
        case ToolType::TEXT_TOOL: {
            m_canvas->SetCursor(wxCursor(wxCURSOR_IBEAM));
            break;
        }
        case ToolType::WIRE_TOOL: {
            wxImage image("res\\icons\\wiring.png", wxBITMAP_TYPE_PNG);
            image.GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_X);
            image.GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_Y);
            wxCursor cursor(image);
            m_canvas->SetCursor(cursor);
            break;
        }
        case ToolType::DRAG_TOOL: {
            m_canvas->SetCursor(wxCursor(wxCURSOR_HAND));
            break;
        }
        default: {
            m_canvas->SetCursor(wxCursor(wxCURSOR_ARROW));
            break;
        }
        }
    }
}

void ToolStateMachine::SetDragState(DragToolState state) {
    if (m_dragState != state) {
        m_dragState = state;
        SendStateChangeEvent(wxString::Format("Drag state changed to %d",
            static_cast<int>(state)));
    }
}

void ToolStateMachine::SetSelectState(SelectToolState state) {
    if (m_selectState != state) {
        m_selectState = state;
        SendStateChangeEvent(wxString::Format("Select state changed to %d",
            static_cast<int>(state)));
    }
}

void ToolStateMachine::SetTextState(TextToolState state) {
    if (m_textState != state) {
        m_textState = state;
        SendStateChangeEvent(wxString::Format("Text state changed to %d",
            static_cast<int>(state)));
    }
}

void ToolStateMachine::SetComponentState(ComponentToolState state) {
    if (m_componentState != state) {
        m_componentState = state;
        SendStateChangeEvent(wxString::Format("Component state changed to %d",
            static_cast<int>(state)));
    }
}

void ToolStateMachine::SetWireState(WireToolState state) {
    if (m_wireState != state) {
        m_wireState = state;
        SendStateChangeEvent(wxString::Format("Wire state changed to %d",
            static_cast<int>(state)));
    }
}

void ToolStateMachine::SetDrawingState(DrawingToolState state) {
    if (m_drawingState != state) {
        m_drawingState = state;
        SendStateChangeEvent(wxString::Format("Drawing state changed to %d",
            static_cast<int>(state)));
    }
}

bool ToolStateMachine::IsIdle() const {
    return m_dragState == DragToolState::IDLE &&
        m_selectState == SelectToolState::IDLE &&
        m_textState == TextToolState::IDLE &&
        m_componentState == ComponentToolState::IDLE &&
        m_wireState == WireToolState::IDLE &&
        m_drawingState == DrawingToolState::IDLE;
}

bool ToolStateMachine::IsCurrentToolIdle() const {
    switch (m_currentTool) {
    case ToolType::DRAG_TOOL:
        return m_dragState == DragToolState::IDLE;
    case ToolType::SELECT_TOOL:
        return m_selectState == SelectToolState::IDLE;
    case ToolType::TEXT_TOOL:
        return m_textState == TextToolState::IDLE;
    case ToolType::COMPONENT_TOOL:
        return m_componentState == ComponentToolState::IDLE;
    case ToolType::WIRE_TOOL:
        return m_wireState == WireToolState::IDLE;
    case ToolType::DRAWING_TOOL:
        return m_drawingState == DrawingToolState::IDLE;
    default:
        return true;
    }
}

bool ToolStateMachine::IsDragging() const {
    return m_dragState != DragToolState::IDLE;
}

bool ToolStateMachine::IsSelecting() const {
    return m_selectState != SelectToolState::IDLE;
}

bool ToolStateMachine::IsEditingText() const {
    return m_textState != TextToolState::IDLE;
}

bool ToolStateMachine::IsPlacingComponent() const {
    return m_componentState == ComponentToolState::COMPONENT_PREVIEW;
}

bool ToolStateMachine::IsDrawingWire() const {
    return m_wireState != WireToolState::IDLE;
}

bool ToolStateMachine::IsDrawingShape() const {
    return m_drawingState != DrawingToolState::IDLE;
}

void ToolStateMachine::PushStateToHistory() {
    ToolStateHistory history;
    history.toolType = m_currentTool;
    history.timestamp = wxDateTime::UNow().GetValue();

    m_stateHistory.push_back(history);

    // 限制历史记录大小
    if (m_stateHistory.size() > MAX_HISTORY_SIZE) {
        m_stateHistory.erase(m_stateHistory.begin());
    }
}

bool ToolStateMachine::Undo() {
    if (m_stateHistory.size() <= 1) {
        return false;
    }

    // 移除当前状态
    m_stateHistory.pop_back();

    // 恢复到上一个状态
    ToolStateHistory previousState = m_stateHistory.back();
    m_currentTool = previousState.toolType;

    // 重置当前工具状态
    ResetCurrentToolState();

    SendStateChangeEvent("State undone");
    return true;
}

void ToolStateMachine::ClearHistory() {
    m_stateHistory.clear();
    PushStateToHistory(); // 至少保留当前状态
}

void ToolStateMachine::ResetAllStates() {
    ResetAllToolStatesToIdle();
    m_currentTool = ToolType::DRAG_TOOL;
    ClearHistory();
    SendStateChangeEvent("All states reset");
}

void ToolStateMachine::ResetCurrentToolState() {

    SetDragState(DragToolState::IDLE);


    SetSelectState(SelectToolState::IDLE);


    SetTextState(TextToolState::IDLE);
 

    SetComponentState(ComponentToolState::IDLE);

    SetWireState(WireToolState::IDLE);

    SetDrawingState(DrawingToolState::IDLE);

}

wxString ToolStateMachine::GetCurrentStateString() const {
    wxString stateStr = wxString::Format("Current Tool: %d, States: [",
        static_cast<int>(m_currentTool));

    stateStr += wxString::Format("Drag:%d, Select:%d, Text:%d, Component:%d, Wire:%d, Drawing:%d]",
        static_cast<int>(m_dragState),
        static_cast<int>(m_selectState),
        static_cast<int>(m_textState),
        static_cast<int>(m_componentState),
        static_cast<int>(m_wireState),
        static_cast<int>(m_drawingState));

    return stateStr;
}

void ToolStateMachine::SendStateChangeEvent(const wxString& stateInfo) {
    wxCommandEvent event(TOOL_STATE_CHANGED);
    event.SetString(stateInfo);
    ProcessEvent(event);
}

void ToolStateMachine::ResetAllToolStatesToIdle() {
    m_dragState = DragToolState::IDLE;
    m_selectState = SelectToolState::IDLE;
    m_textState = TextToolState::IDLE;
    m_componentState = ComponentToolState::IDLE;
    m_wireState = WireToolState::IDLE;
    m_drawingState = DrawingToolState::IDLE;
}