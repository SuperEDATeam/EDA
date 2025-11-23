#pragma once
#include <wx/wx.h>
#include <vector>
#include <wx/graphics.h>
#include "CanvasElement.h"
#include "Wire.h"          // ← 新增：连线数据
#include "UndoStack.h"
#include "HandyToolKit.h"
#include "ToolStateMachine.h"
#include "CanvasEventHandler.h"

class HandyToolKit;
class CanvasEventHandler;
class MainFrame;
class CanvasTextElement;
class ToolStateMachine;

/* 新增：拖动时需要更新的连线索引 + 对应引脚信息 */
struct WireAnchor {
    size_t wireIdx;   // 哪条线
    size_t ptIdx;     // 该线第几个控制点（0 或 最后）
    bool   isInput;   // 元件侧是输入还是输出引脚
    size_t pinIdx;    // 元件侧引脚索引
};

struct HoverInfo {
    wxPoint pos;

    // -----------------
    // 1. 引脚 (Pin) 悬停信息
    // -----------------
    int pinIndex = -1;         // 悬停引脚的索引。-1 表示没有悬停在任何引脚上。
	bool isInputPin = false;   // 悬停引脚是否为输入引脚。
    wxPoint pinWorldPos;       // 悬停引脚在世界坐标系中的位置。

    // -----------------
    // 2. 导线控制点 (Cell/Wire Control Point) 悬停信息
    // -----------------
    int cellIndex = -1;        // 悬停导线控制点的索引。-1 表示没有悬停在控制点上。
    int wireIndex = -1;        // 控制点所属导线的索引。
    wxPoint cellWorldPos;      // 控制点在世界坐标系中的位置。

    // -----------------
    // 3. 元件 (Element) 悬停信息 (用于状态栏反馈)
    // -----------------
    int elementIndex = -1;     // 悬停元件的索引。-1 表示没有悬停在任何元件上。
    wxString elementName;      // 悬停元件的名称。

    // 辅助函数，判断是否悬停在某个具体对象上
    bool IsOverPin() const { return pinIndex != -1; }
    bool IsOverCell() const { return cellIndex != -1; }
    bool IsOverElement() const { return elementIndex != -1; }

    // 构造函数
	HoverInfo() : pinIndex(-1), isInputPin(false), cellIndex(-1), wireIndex(-1), elementIndex(-1) {}
};

class CanvasPanel : public wxPanel
{
    std::vector<CmdMoveElement::Anchor> m_undoAnchors;
    wxPoint m_dragStartElemPos;        // 拖动前元件左上角
    size_t m_wireCountBeforeOperation = 0;  // 操作前的导线数量
public:
    CanvasPanel(MainFrame* parent);
    void AddElement(const CanvasElement& elem);
    void PlaceElement(const wxString& name, const wxPoint& pos);

    UndoStack m_undoStack;

	MainFrame* m_mainFrame;

    // 缩放相关方法
    float GetScale() const { return m_scale; }  // 获取当前缩放比例
    void SetScale(float scale);                 // 设置缩放比例并刷新

    // 坐标转换（屏幕坐标 <-> 画布坐标，考虑缩放）
    wxPoint ScreenToCanvas(const wxPoint& screenPos) const;
    wxPoint CanvasToScreen(const wxPoint& canvasPos) const;

    wxPoint m_offset;          // 画布偏移量（平移坐标）
    bool m_isPanning;          // 是否正在拖拽平移
    wxPoint m_panStartPos;     // 平移开始时的屏幕坐标


      // 新增：判断是否点击在空白区域（无元素）
    bool IsClickOnEmptyArea(const wxPoint& canvasPos);

    // 事件处理函数（将中键改为左键相关）
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt);  
    void OnRightDown(wxMouseEvent& evt);
    void OnRightUp(wxMouseEvent& evt);
    void OnCursorTimer(wxTimerEvent& event);

    const std::vector<CanvasElement>& GetElements() const { return m_elements; }
    void ClearAll() {
        m_elements.clear();
        m_wires.clear();
        m_selectedIndex = -1;
        Refresh();
    }

    void AddWire(const Wire& wire) {
        m_wires.push_back(wire);
        Refresh();
    }

    void CollectUndoAnchor(size_t elemIdx, std::vector<CmdMoveElement::Anchor>& out);

    // 暴露连线容器，供外部仿真/导出使用
    const std::vector<Wire>& GetWires() const { return m_wires; }


    void DeleteSelectedElement();

    // 添加双击事件处理
    void OnLeftDoubleClick(wxMouseEvent& evt);

    // 添加定时器相关成员（用于区分单击和双击）
    wxTimer m_clickTimer;
    wxPoint m_clickPos;
    int m_clickElementIndex = -1;
    void OnClickTimer(wxTimerEvent& evt);


public:
    // 文本元素相关
    std::vector<CanvasTextElement> m_textElements;
    wxTimer m_cursorTimer;
	int inWhichTextBox(const wxPoint& canvasPos);
    void CreateTextElement(const wxPoint& position);
	void SetFocusToTextElement(int index);

    wxTextCtrl* m_hiddenTextCtrl;
    int m_currentEditingTextIndex;

    void SetupHiddenTextCtrl();
    void AttachHiddenTextCtrlToElement(int textIndex);
    void DetachHiddenTextCtrl();

    /* ---------- 原有元件相关 ---------- */
    std::vector<CanvasElement> m_elements;
    int  m_selectedIndex = -1;
    bool m_isDragging = false;
    wxPoint m_dragStartPos;
    wxPoint m_elementStartPos;


    void ClearElementWires(size_t elemIndex);


    std::vector<Wire> m_wires;   // 已定型的连线
    Wire m_tempWire;             // 正在拖动/预览的连线
    enum class WireMode { Idle, DragNew, DragMove };
    WireMode m_wireMode = WireMode::Idle;

    ControlPoint m_startCP;      // 连线起点
    wxPoint m_curSnap;           // 当前吸附/预览点


    float m_scale = 1.0f;


    void OnPaint(wxPaintEvent& evt);
    int  HitTest(const wxPoint& pt);


    // 吸附：网格 + 引脚
    wxPoint Snap(const wxPoint& raw, bool* snapped);

    std::vector<WireAnchor> m_movingWires;

    int  HitHoverPin(const wxPoint& raw, bool* isInput, wxPoint* worldPos);

    int HitHoverCell(const wxPoint& raw, int* wireIdx, int* cellIdx, wxPoint* cellPos);

	// 工具管理器需要的接口
public:
    void ClearSelection();
    void SetSelectedIndex(int index);
    int HitTestPublic(const wxPoint& pt);
    bool IsClickOnEmptyAreaPublic(const wxPoint& canvasPos);

    // 快捷工具栏
    HandyToolKit* m_HandyToolKit;

    // 工具状态机
	ToolStateMachine* m_toolStateMachine;

	// 工具管理器
    CanvasEventHandler* m_CanvasEventHandler;

    // 更新状态栏方法
    void SetStatus(wxString status);

	// 对外的工具状态接口
	void SetCurrentTool(ToolType tool);
	void SetCurrentComponent(const wxString& componentName);

    // 悬停信息
	HoverInfo m_hoverInfo;

    // 悬停信息检测
    void UpdateHoverInfo(const wxPoint& canvasPos);

    std::vector<WireWireAnchor> m_wireWireAnchors;// 导线<->导线小方块（新增）
private:
    // 添加焦点状态
    bool m_hasFocus;

    // 添加焦点事件处理
    void OnFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void EnsureFocus();


public:
    // 隐藏的文本控件用于输入法支持
    bool m_isUsingHiddenCtrl;

    void StartTextEditing(int index);

    wxTextCtrl* m_sharedTextCtrl;

    wxDECLARE_EVENT_TABLE();
};