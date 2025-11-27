#pragma once
#include <wx/wx.h>
#include "ToolBars.h"
#include "CanvasPanel.h"
#include "ToolStateMachine.h" // 包含状态机头文件

class MainFrame;
class CanvasPanel;
class ToolBars;
// class ToolStateMachine; // 不需要前向声明了，因为已经 include 了

// 删除 ToolType 枚举定义，直接使用 ToolStateMachine.h 中的定义

class CanvasEventHandler {
private:
    CanvasPanel* m_canvas;
    bool m_eventHandled;

    // 保存一个标记，用于判断是否是从其他工具临时切换过来的（例如在默认模式下临时画线）
    // 如果你想完全依赖状态机，可以去掉这个，但为了保留你原有的交互体验（画完一根线自动切回），建议保留这个逻辑变量
    bool m_isTemporaryAction;
    ToolType m_previousTool;

    // 具体的绘制/操作数据仍然需要保留，因为状态机只管“状态”，不管“数据”
    wxPoint m_wireStartPos;
    ControlPoint m_startCP;
    ControlPoint m_endCP;

    std::vector<WireAnchor> m_movingWires;

    wxPoint m_panStartPos;
    wxPoint m_fakeStartPos;

    int m_editingWireIndex;
    int m_editingPointIndex;
    wxPoint m_editStartPos;

    int m_draggingElementIndex;
    wxPoint m_elementDragStartPos;
    wxPoint m_elementStartCanvasPos;

    // 当前选中的元件名（用于放置）
    wxString m_currentComponent;

    // 核心：持有状态机指针
    ToolStateMachine* m_toolStateMachine;

	// 文本框编辑相关
    int m_editingTextIndex;
    std::map<wxTextCtrl*, int> m_textCtrlBindings;

public:
    // 选中工具相关
    std::vector<int> m_textElemIdx;
    std::vector<int> m_compntIdx;
    std::vector<int> m_wireIdx;
	wxPoint m_selectStartPos;
    wxPoint m_selectedDragPos;

    std::vector<wxPoint> m_textElemPos;
    std::vector<wxPoint> m_compntPos;
    std::vector<std::vector<wxPoint>> m_wirePos;

    int m_selectedIndex;
    int m_tmpwireIdx;
    int m_textIdx;
    bool preIn;

public:
    
    
    
    
    CanvasEventHandler(CanvasPanel* canvas, ToolStateMachine* toolstate);


	// 文本框
    void BindHiddenTextCtrlEvents();
    void FinishTextEditing();
    void StartTextEditing(int index);
    void CreateTextElement(const wxPoint& position);
    void BindTextCtrlEvents(wxTextCtrl* textCtrl, int textIndex);
    void OnTextCtrlTextChanged(wxCommandEvent& evt, int textIndex);
    void OnTextCtrlEnter(wxCommandEvent& evt, int textIndex);
    void OnTextCtrlKillFocus(wxFocusEvent& evt, int textIndex);
    void OnHiddenTextCtrlText(wxCommandEvent& evt);
    void OnHiddenTextCtrlEnter(wxCommandEvent& evt);
    void OnHiddenTextCtrlKillFocus(wxFocusEvent& evt);
    void UnbindTextCtrlEvents(wxTextCtrl* textCtrl);

    // 代理状态机的 SetCurrentTool，并处理一些本地逻辑
    void SetCurrentTool(ToolType tool);
    void SetCurrentTool4Bar(ToolType tool);

    bool IsEventHandled() const { return m_eventHandled; }
    void ResetEventHandled() { m_eventHandled = false; }
    void CleanTempTool();

    // 工具行为方法
    void HandleSelectTool(wxMouseEvent& evt);
    void HandleTextTool(const wxPoint& canvasPos);
    void HandleComponentTool(const wxPoint& canvasPos);
    void HandleWireTool(const wxPoint& canvasPos);
    void HandleDragTool(const wxPoint& canvasPos);
    // HandleDefaultTool 可以移除，逻辑并入 OnCanvasLeftDown 的 DRAG_TOOL 处理中

    // 画布事件转发
    void OnCanvasLeftDown(wxMouseEvent& evt);
    void OnCanvasLeftUp(wxMouseEvent& evt);
    void OnCanvasMouseMove(wxMouseEvent& evt);
    void OnCanvasKeyDown(wxKeyEvent& evt);
    void OnCanvasMouseWheel(wxMouseEvent& evt);
    void OnCanvasRightDown(wxMouseEvent& evt);
    void OnCanvasRightUp(wxMouseEvent& evt);
    bool IsCharacterKey(const wxKeyEvent& event);

    // 动作开启/更新/结束方法
    void StartWireDrawing(const wxPoint& startPos, CPType startType);
    void UpdateWireDrawing(const wxPoint& currentPos);
    void FinishWireDrawing(const wxPoint& endPos);
    void CancelWireDrawing();

    void StartPanning(const wxPoint& startPos);
    void UpdatePanning(const wxPoint& currentPos);
    void FinishPanning(const wxPoint& currentPos);

    void StartWireEditing(int wireIndex, int pointIndex, const wxPoint& startPos);
    void UpdateWireEditing(const wxPoint& currentPos);
    void FinishWireEditing();
    void CancelWireEditing();

    void StartElementDragging(int elementIndex, const wxPoint& startPos);
    void UpdateElementDragging(const wxPoint& currentPos);
    void FinishElementDragging();

    void SetCurrentComponent(const wxString& componentName);

    void StartSelectedDragging(const wxPoint& startPos);
    //void UpdateSelectedDragging(const wxPoint& startPos);
    //void FinishSelectedDragging(const wxPoint& startPos);
    void _StartElementDragging(int elementIndex);
};