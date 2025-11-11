#pragma once
#include <wx/wx.h>
#include "ToolBars.h"
#include "CanvasPanel.h"

class MainFrame; // 前向声明
class CanvasPanel; // 前向声明
class ToolBars;   // 前向声明

enum class ToolType {
    DEFAULT_TOOL,    // 默认工具（包含导线绘制、平移等）
    SELECT_TOOL,     // 选中工具
    TEXT_TOOL,       // 文本工具
    COMPONENT_TOOL,  // 元件工具
    WIRE_TOOL,       // 专门的导线工具
    DRAG_TOOL         // 拖拽工具
};

class ToolManager {
private:
    ToolType m_currentTool;
	ToolType m_previousTool;
    bool m_tempTool;
    ToolBars* m_toolBars;
    CanvasPanel* m_canvas;
    MainFrame* m_mainFrame;
    wxString m_currentComponent;
    bool m_eventHandled;

    // 导线绘制相关状态
    bool m_isDrawingWire;
    wxPoint m_wireStartPos;
    ControlPoint m_startCP;
    wxPoint m_fixedStartPos;

    // 平移相关状态
    bool m_isPanning;
    wxPoint m_panStartPos;
	wxPoint m_fakeStartPos;

    // 导线编辑相关状态
    bool m_isEditingWire;
    int m_editingWireIndex;
    int m_editingPointIndex;
    wxPoint m_editStartPos;

    // 元件拖动相关状态
    bool m_isDraggingElement;
    int m_draggingElementIndex;
    wxPoint m_elementDragStartPos;
    wxPoint m_elementStartCanvasPos;

    // 文本编辑相关状态
    int m_editingTextIndex = -1;

public:
    ToolManager(MainFrame* mainFrame, ToolBars* toolBars, CanvasPanel* canvas);

    void SetCurrentTool(ToolType tool);
    void SetCurrentTool4Bar(ToolType tool);
    ToolType GetCurrentTool() const { return m_currentTool; }
    void SetCurrentComponent(const wxString& componentName);

    bool IsEventHandled() const { return m_eventHandled; }
    void ResetEventHandled() { m_eventHandled = false; }
    void CleanTempTool();

    // 工具行为方法
    void HandleDefaultTool(const wxPoint& canvasPos);
    void HandleSelectTool(const wxPoint& canvasPos);
    void HandleTextTool(const wxPoint& canvasPos);
    void HandleComponentTool(const wxPoint& canvasPos);
    void HandleWireTool(const wxPoint& canvasPos);
    void HandleDragTool(const wxPoint& canvasPos);

    // 画布事件转发
    void OnCanvasLeftDown(const wxPoint& canvasPos);
    void OnCanvasLeftUp(const wxPoint& canvasPos);
    void OnCanvasMouseMove(const wxPoint& canvasPos);
    void OnCanvasKeyDown(wxKeyEvent& evt);
    void OnCanvasMouseWheel(wxMouseEvent& evt);
    void OnCanvasRightDown(const wxPoint& canvasPos);
    void OnCanvasRightUp(const wxPoint& canvasPos);
    bool IsCharacterKey(const wxKeyEvent& event);
    void OnCursorTimer(wxTimerEvent& event);

    // 导线绘制方法
    void StartWireDrawing(const wxPoint& startPos, bool fromPin);
    void UpdateWireDrawing(const wxPoint& currentPos);
    void FinishWireDrawing(const wxPoint& endPos);
    void CancelWireDrawing();

    // 平移方法
    void StartPanning(const wxPoint& startPos);
    void UpdatePanning(const wxPoint& currentPos);
    void FinishPanning(const wxPoint& currentPos);

    // 状态获取
    bool IsDrawingWire() const { return m_isDrawingWire; }
    bool IsPanning() const { return m_isPanning; }
    ControlPoint GetStartControlPoint() const { return m_startCP; }

    // 导线编辑方法
    void StartWireEditing(int wireIndex, int pointIndex, const wxPoint& startPos);
    void UpdateWireEditing(const wxPoint& currentPos);
    void FinishWireEditing();
    void CancelWireEditing();

    // 元件拖动方法
    void StartElementDragging(int elementIndex, const wxPoint& startPos);
    void UpdateElementDragging(const wxPoint& currentPos);
    void FinishElementDragging();
    
    // 文本编辑方法
    void CreateTextElement(const wxPoint& position);
    void StartTextEditing(int index);
    void FinishTextEditing();

    // 状态获取
    bool IsEditingWire() const { return m_isEditingWire; }
    bool IsDraggingElement() const { return m_isDraggingElement; }

    void HandleHoverFeedback(const wxPoint& canvasPos);

};