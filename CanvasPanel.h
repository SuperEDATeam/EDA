#pragma once
#include <wx/wx.h>
#include <vector>
#include <chrono>
#include <wx/graphics.h>

#include "CanvasElement.h"
#include "Wire.h"
#include "UndoStack.h"
#include "CanvasTextElement.h"
#include "ToolStateMachine.h"

class HandyToolKit;
class CanvasEventHandler;
class MainFrame;

struct HoverInfo {
    wxPoint pos;
    wxPoint snappedPos; // ���ȼ������ţ����߿��Ƶ�

    // 1. 引脚 (Pin) 悬停信息
    int pinIndex = -1;         // 悬停引脚的索引。-1 表示没有悬停在任何引脚上。
	bool isInputPin = false;   // 悬停引脚是否为输入引脚。
    wxPoint pinWorldPos;       // 悬停引脚在世界坐标系中的位置。

    // 2. 导线控制点 (Cell/Wire Control Point) 悬停信息
    int cellIndex = -1;        // 悬停导线控制点的索引。-1 表示没有悬停在控制点上。
    int wireIndex = -1;        // 控制点所属导线的索引。
    wxPoint cellWorldPos;      // 控制点在世界坐标系中的位置。

    // 3. 元件 (Element) 悬停信息 (用于状态栏反馈)
    int elementIndex = -1;     // 悬停元件的索引。-1 表示没有悬停在任何元件上。
    wxString elementName;      // 悬停元件的名称。

	// 4. 文本框 (Text Element) 悬停信息
	int textIndex = -1; // 悬停文本框的索引。-1 表示没有悬停在任何文本框上。

    // 辅助函数，判断是否悬停在某个具体对象上
    bool IsOverPin() const { return pinIndex != -1; }
    bool IsOverCell() const { return cellIndex != -1; }
    bool IsOverElement() const { return elementIndex != -1; }
	bool IsOverText() const { return textIndex != -1; }
    bool IsEmptyArea() const {
        return pinIndex == -1 && cellIndex == -1 && elementIndex == -1 && textIndex == -1;
	}

    // 构造函数
	HoverInfo() : pinIndex(-1), isInputPin(false), cellIndex(-1), wireIndex(-1), elementIndex(-1), textIndex(-1){}
};

class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(MainFrame* parent, size_t size_x, size_t size_y);

    // ==================== 画布及画布子窗口的事件处理 ====================
private:
    bool m_hasFocus;

    // 鼠标事件
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnLeftDoubleClick(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnRightDown(wxMouseEvent& evt);
    void OnRightUp(wxMouseEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt);

    // 键盘事件
    void OnKeyDown(wxKeyEvent& evt);

    // 焦点事件
    void OnFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void EnsureFocus();

    // 绘制和滚动事件
    void OnPaint(wxPaintEvent& evt);
    void OnScroll(wxScrollEvent& event);

    // ==================== 画布中元素管理 ====================
private:
    // 元件和连线数据
    std::vector<CanvasElement> m_elements;
    std::vector<CanvasTextElement> m_textElements;
    std::vector<Wire> m_wires;

    // 选中索引
    wxRect m_selectRect; // 选中矩形框
    std::vector<int> m_selTxtIdx;
    std::vector<int> m_selElemIdx;
    std::vector<int> m_selWireIdx;

    // 预览元素
    CanvasElement m_previewElement;
    Wire m_previewWire;

    // 文本编辑
    wxTextCtrl* m_hiddenTextCtrl;
    wxTextCtrl* m_sharedTextCtrl;
    int m_currentEditingTextIndex;
    bool m_isUsingHiddenCtrl;

public:
    void ClearAll();
    void DeleteSelected();

    // 元件管理
    const std::vector<CanvasElement>& GetElements() const { return m_elements; }
    void AddElement(const wxString& name, const wxPoint& pos);
    void AddElementWithoutRecord(const wxString& name, const wxPoint& pos);
    void DeleteElement(int index) { m_elements.erase(m_elements.begin() + index);  m_selElemIdx.erase(std::remove(m_selElemIdx.begin(), m_selElemIdx.end(), index), m_selElemIdx.end()); Refresh(); };
    void ElementSetPos(int index, const wxPoint& pos);
    void ElementStatusChange(int index);

    // 导线管理
    const std::vector<Wire>& GetWires() const { return m_wires; }
    void AddWire(const Wire& wire); 
    void AddWireWithoutRecord(const Wire& wire);
    void DeleteWire(int index) { m_wires.erase(m_wires.begin() + index); m_selWireIdx.erase(std::remove(m_selWireIdx.begin(), m_selWireIdx.end(), index), m_selWireIdx.end()); Refresh(); };
    void WireSetWholeOffSet(int index, const wxPoint& offset);
    void WirePtsSetPos(int wireIndex, int controlPointIndex, const wxPoint& pos);
    void WireGenerateCells(int Index) { m_wires[Index].GenerateCells(); };
    void UpdateWire(const Wire& newWire, int index) {m_wires[index] = newWire; m_wires[index].GenerateCells(); Refresh(); };
    void UpdatePreviewWire(Wire& wire) { m_previewWire = wire; Refresh(); };
    void ClearPreviewWire() { m_previewWire = Wire(); Refresh(); };

    // 文本管理
    const std::vector<CanvasTextElement>& GetTextElements() const { return m_textElements; }
    void CreateTextElement(const wxPoint& position, wxString text);
    void CreateTextElementWithoutRecord(const wxPoint& position, wxString text);
    void DeleteTextElement(int index) { m_textElements.erase(m_textElements.begin() + index); m_selTxtIdx.erase(std::remove(m_selTxtIdx.begin(), m_selTxtIdx.end(), index), m_selTxtIdx.end()); Refresh(); };
    void TextSetPos(int index, const wxPoint& pos) { m_textElements[index].SetPosition(pos); Refresh(); };
    void StartTextEditing(int index);
    void FinishTextEditing() { DetachHiddenTextCtrl(); m_toolStateMachine->SetTextState(TextToolState::IDLE); };
    void SetupHiddenTextCtrl();
    void AttachHiddenTextCtrlToElement(int textIndex);
    void DetachHiddenTextCtrl();
    void OnHiddenTextCtrlEnter(wxCommandEvent& evt) { FinishTextEditing(); };
    void OnHiddenTextCtrlKillFocus(wxFocusEvent& evt) { FinishTextEditing(); evt.Skip(); };

    // 预览元素管理
    void SetPreviewElement(const wxString& name, wxPoint pos);

    // 选中元素管理
    void UpdateSelection(std::vector<int> m_textIdx, std::vector<int> m_elemIdx, std::vector<int> m_wireIdx);
    void ClearSelection() { m_selTxtIdx.clear(); m_selElemIdx.clear(); m_selWireIdx.clear(); Refresh(); };

    // 选择矩形框框管理
    const wxRect GetSelectionRect() const { return m_selectRect; };
    void SetSelectionRect(const wxRect& rect) { m_selectRect = rect; Refresh(); };
    void ClearSelectionRect() { m_selectRect = wxRect(); Refresh(); };
    // ==================== 画布本身的大小位置定义和坐标转换 ====================

private:
    // 画布状态
    wxSize m_size;
    int m_grid;
    wxPoint m_offset;
    float m_scale = 1.0f;

public:
    // 画布的缩放
    float GetScale() const { return m_scale; }
    void SetScale(float scale);
    std::pair<float, float> ValidScaleRange();

    // 偏移和滚动
    const wxPoint GetoffSet() const { return m_offset; };
    void SetoffSet(wxPoint offset);
    std::pair<wxPoint, wxPoint> ValidSetOffRange();

    // 网格大小
    int GetGrid() const { return m_grid; };
    void SetGrid(int grid) { m_grid = grid; };

    // 坐标转换
    wxPoint ScreenToCanvas(const wxPoint& screenPos) const;
    wxPoint CanvasToScreen(const wxPoint& canvasPos) const;
    wxPoint LogicToDevice(const wxPoint& logicPoint) const;
    wxPoint DeviceToLogic(const wxPoint& devicePoint) const;

    // ==================== 画布键鼠工具状态管理 ====================
private:
    HoverInfo m_hoverInfo;

    void UpdateHoverInfo(const wxPoint& canvasPos);
    int HitElementTest(const wxPoint& canvasPos);
    int HitTestText(wxPoint canvasPos);
    int HitHoverPin(const wxPoint& canvasPos, bool* isInput, wxPoint* worldPos);
    int HitHoverCell(const wxPoint& canvasPos, int* wireIdx, int* cellIdx, wxPoint* cellPos);
    wxPoint Snap(const wxPoint& canvasPos) { return wxPoint((canvasPos.x + m_grid / 2) / m_grid * m_grid, (canvasPos.y + m_grid / 2) / m_grid * m_grid); };

public:
    // 工具管理
    void SetCurrentTool(ToolType tool);
    void SetCurrentComponent(const wxString& componentName);

    // ==================== 画布的子系统 ====================
private:
    // 撤销栈
    UndoStack m_undoStack;

    // 工具系统
    ToolStateMachine* m_toolStateMachine;
    CanvasEventHandler* m_CanvasEventHandler;
    HandyToolKit* m_HandyToolKit;

    // 滚动条
    wxScrollBar* m_hScroll;
    wxScrollBar* m_vScroll;

    void LayoutScrollbars();

public:
    // 工具相关获取方法
    ToolStateMachine* GetToolStateMachine() const { return m_toolStateMachine; }
    CanvasEventHandler* GetCanvasEventHandler() const { return m_CanvasEventHandler; }

    // 撤销栈相关
    void UndoStackPush(std::unique_ptr<Command> command);
    void UndoStackUndo() { m_undoStack.Undo(this); };
    const wxString UndoStackGetUndoName() const { return m_undoStack.GetUndoName(); };
    bool UndoStackCanUndo() const { return m_undoStack.CanUndo(); };
    

    // ==================== 父窗口 ====================
private:
    MainFrame* m_mainFrame;

public:
    // 状态栏更新
    void SetStatus(wxString status);
    
    // ==================== 事件表 ====================
    wxDECLARE_EVENT_TABLE();
};