#pragma once
#include <wx/wx.h>
#include <map>
#include "ToolStateMachine.h"
#include "Wire.h"
#include "UndoStack.h"
#include "CanvasPanel.h"

class CanvasEventHandler {
    // ==================== 构造函数和基础方法 ====================
public:
    CanvasEventHandler(CanvasPanel* canvas, ToolStateMachine* toolstate);

    bool IsEventHandled() const { return m_eventHandled; }
    void ResetEventHandled() { m_eventHandled = false; }

    // ==================== 画布事件处理接口 ====================
public:
    void OnCanvasLeftDown(wxMouseEvent& evt);
    void OnCanvasLeftUp(wxMouseEvent& evt);
    void OnCanvasLeftDoubleClick(wxMouseEvent& evt);
    void OnCanvasMouseMove(wxMouseEvent& evt);
    void OnCanvasKeyDown(wxKeyEvent& evt);
    void OnCanvasMouseWheel(wxMouseEvent& evt);

    // ==================== 工具管理和状态控制 ====================
public:
    void SetCurrentTool(ToolType tool);
    void SetCurrentComponent(const wxString& componentName);

    // ==================== 画布平移事件处理 ====================
private:
    wxPoint m_panStartPos;
    wxPoint m_panStartOffSet;

    void StartPanning(const wxPoint& startPos);
    void UpdatePanning();
    void FinishPanning();

    // ==================== 选择工具画布事件处理 ====================
private:
    // 选择相关数据
    std::vector<int> m_textElemIdx;
    std::vector<int> m_compntIdx;
    std::vector<int> m_wireIdx;

    std::vector<int> m_textRecElemIdx;
    std::vector<int> m_compntRecIdx;
    std::vector<int> m_wireRecIdx;

    std::vector<wxPoint> m_textElemPos;
    std::vector<wxPoint> m_compntPos;
    std::vector<std::vector<wxPoint>> m_wirePos;

    wxPoint m_selectStartPos;
    wxPoint m_selectedDragPos;

    std::vector<std::vector<WireAnchor>> m_movingWires;

    bool preIn;

    void HandleSelectTool(wxMouseEvent& evt);

    // 矩形选择相关操作
    void StartRectangleSelect(const wxPoint& startPos);
    void UpdateRectangleSelect(wxMouseEvent& evt);
    void FinishRectangleSelect();

    // 选中拖拽相关操作
    void StartSelectedDragging(const wxPoint& startPos);
    void StartElementDragging(int index);
    void UpdateSelectedDragging();

    // 单击选中相关操作
    void FinishClickSelect(wxMouseEvent& evt);

    // ==================== 元件工具画布事件处理 ====================
private:
    wxString m_currentComponent;
    void HandleComponentTool();

    // ==================== 文本工具画布事件处理 ====================
private:
    void HandleTextTool();

    // ==================== 导线工具画布事件处理 ====================
private:
    int lengthOfTempWire;
    int tempWirePlus;
    bool m_isWireDraingCancel;

    void StartWireDrawingDown(const wxPoint& startPos, CPType startType);
    void StartWireDrawingUp();
    void UpdateWireDrawing(wxMouseEvent& evt);
    void PlaceWirePoint();
    void FinishWireDrawing();
    void CancelWireDrawing();

public:
    // 各工具的核心处理逻辑
   
    

	// 导线工具相关操作
    

    void StartWireEditing(int wireIndex, int pointIndex, const wxPoint& startPos);
    void UpdateWireEditing(const wxPoint& currentPos);
    void FinishWireEditing();
    void CancelWireEditing();

    



    // 文本编辑相关操作
    void StartTextEditing(int index);
    void UpdateHoverInfo(HoverInfo hf);

    // ==================== 公共数据成员 ====================
public:
    // 临时状态数据
    Wire m_tempWire;
    HoverInfo m_hoverInfo;



    // 位置备份（用于撤销等）


    bool m_snapPosChanged;
    bool SnapPosChanged() { return m_snapPosChanged; };
    // ==================== 私有实现细节 ====================
private:
    // 核心引用和状态
    CanvasPanel* m_canvas;
    ToolStateMachine* m_toolStateMachine;
    bool m_eventHandled;

    // 工具切换状态
    bool m_isTemporaryAction;
    ToolType m_previousTool;
    


  

    // 连线编辑状态
    int m_editingWireIndex;
    int m_editingPointIndex;
    wxPoint m_editStartPos;

    // 元件拖拽状态
    int m_draggingElementIndex;
    wxPoint m_elementDragStartPos;
    wxPoint m_elementStartCanvasPos;

    // 文本编辑状态
    int m_editingTextIndex;
    std::map<wxTextCtrl*, int> m_textCtrlBindings;
};