#pragma once

#include <wx/event.h>
#include <memory>
#include <vector>

class CanvasPanel;

// 工具类型枚举
enum class ToolType {
    DRAG_TOOL,        // 拖拽工具（默认工具）
    SELECT_TOOL,      // 选中工具
    TEXT_TOOL,        // 文本工具  
    COMPONENT_TOOL,   // 元件放置工具
    WIRE_TOOL,        // 导线绘制工具
    DRAWING_TOOL      // 绘图工具
};

// 拖拽工具状态
enum class DragToolState {
    IDLE,
    CANVAS_DRAGGING,        // 画布拖动
    COMPONENT_DRAGGING,     // 元件拖动
    WIRE_WHOLE_DRAGGING,    // 整体导线拖动
    WIRE_SEGMENT_DRAGGING,  // 导线段拖动
    TEXT_DRAGGING          // 文本拖动
};

// 选中工具状态
enum class SelectToolState {
    IDLE,
    CLICK_SELECT,           // 点击选择
    RECTANGLE_SELECT,       // 矩形框选
    MULTI_SELECT           // 多选状态
};

// 文本工具状态
enum class TextToolState {
    IDLE,
    CANVAS_TEXT_EDITING,    // 画布文本编辑
    COMPONENT_LABEL_EDITING // 元件标签编辑
};

// 元件工具状态
enum class ComponentToolState {
    IDLE,
    COMPONENT_PREVIEW       // 元件预览
};

// 导线工具状态
enum class WireToolState {
    IDLE,
    WIRE_DRAWING,           // 导线绘制中
	WIRE_EDITING,		   // 导线编辑中
    WIRE_BRANCHING
};

// 绘图工具状态
enum class DrawingToolState {
    IDLE,
    DRAWING_RECTANGLE,      // 绘制矩形
    DRAWING_LINE,           // 绘制直线
    DRAWING_CIRCLE         // 绘制圆形
};

// 状态历史记录（用于撤回）
struct ToolStateHistory {
    ToolType toolType;
    wxLongLong timestamp;
    // 可以扩展其他需要记录的状态信息
};

// 工具状态改变事件
wxDECLARE_EVENT(TOOL_STATE_CHANGED, wxCommandEvent);

class ToolStateMachine : public wxEvtHandler {
public:
    ToolStateMachine(CanvasPanel* canvas);
    ~ToolStateMachine();

	CanvasPanel* m_canvas;
    
    // 主工具切换接口
    void SetCurrentTool(ToolType tool);
    ToolType GetCurrentTool() const { return m_currentTool; }

    // 拖拽工具状态管理
    void SetDragState(DragToolState state);
    DragToolState GetDragState() const { return m_dragState; }

    // 选中工具状态管理  
    void SetSelectState(SelectToolState state);
    SelectToolState GetSelectState() const { return m_selectState; }

    // 文本工具状态管理
    void SetTextState(TextToolState state);
    TextToolState GetTextState() const { return m_textState; }

    // 元件工具状态管理
    void SetComponentState(ComponentToolState state);
    ComponentToolState GetComponentState() const { return m_componentState; }

    // 导线工具状态管理
    void SetWireState(WireToolState state);
    WireToolState GetWireState() const { return m_wireState; }

    // 绘图工具状态管理
    void SetDrawingState(DrawingToolState state);
    DrawingToolState GetDrawingState() const { return m_drawingState; }

    // 便捷状态查询方法
    bool IsIdle() const;
    bool IsCurrentToolIdle() const;
    bool IsDragging() const;
    bool IsSelecting() const;
    bool IsEditingText() const;
    bool IsPlacingComponent() const;
    bool IsDrawingWire() const;
    bool IsDrawingShape() const;

    // 工具类型快速查询
    bool IsDragTool() const { return m_currentTool == ToolType::DRAG_TOOL; }
    bool IsSelectTool() const { return m_currentTool == ToolType::SELECT_TOOL; }
    bool IsTextTool() const { return m_currentTool == ToolType::TEXT_TOOL; }
    bool IsComponentTool() const { return m_currentTool == ToolType::COMPONENT_TOOL; }
    bool IsWireTool() const { return m_currentTool == ToolType::WIRE_TOOL; }
    bool IsDrawingTool() const { return m_currentTool == ToolType::DRAWING_TOOL; }

    // 状态历史管理（用于撤回）
    void PushStateToHistory();
    bool CanUndo() const { return m_stateHistory.size() > 1; }
    bool Undo();
    void ClearHistory();

    // 状态重置
    void ResetAllStates();
    void ResetCurrentToolState();

    // 获取当前工具状态的字符串表示（用于调试）
    wxString GetCurrentStateString() const;

private:
    ToolType m_currentTool;

    // 各工具状态
    DragToolState m_dragState;
    SelectToolState m_selectState;
    TextToolState m_textState;
    ComponentToolState m_componentState;
    WireToolState m_wireState;
    DrawingToolState m_drawingState;

    // 状态历史（用于撤回）
    std::vector<ToolStateHistory> m_stateHistory;
    static const int MAX_HISTORY_SIZE = 50;

    // 发送状态改变事件
    void SendStateChangeEvent(const wxString& stateInfo = "");

    // 重置所有工具状态到空闲
    void ResetAllToolStatesToIdle();
};

