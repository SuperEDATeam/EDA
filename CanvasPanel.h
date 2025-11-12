#pragma once
#include <wx/wx.h>
#include <vector>
#include <wx/graphics.h>
#include "CanvasElement.h"
#include "Wire.h"          // ← 新增：连线数据
#include "QuickToolBar.h"
#include "UndoStack.h"

class QuickToolBar;
class ToolManager;
class MainFrame;

/* 新增：拖动时需要更新的连线索引 + 对应引脚信息 */
struct WireAnchor {
    size_t wireIdx;   // 哪条线
    size_t ptIdx;     // 该线第几个控制点（0 或 最后）
    bool   isInput;   // 元件侧是输入还是输出引脚
    size_t pinIdx;    // 元件侧引脚索引
};


class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(wxWindow* parent);
    void AddElement(const CanvasElement& elem);
    void PlaceElement(const wxString& name, const wxPoint& pos);

    UndoStack m_undoStack;

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



    // 暴露连线容器，供外部仿真/导出使用
    const std::vector<Wire>& GetWires() const { return m_wires; }


    void DeleteSelectedElement();


public:
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

    int m_hoverPinIdx=-1;
    bool m_hoverIsInput=false;
    wxPoint m_hoverPinPos;
    int  HitHoverPin(const wxPoint& raw, bool* isInput, wxPoint* worldPos);

    int  m_hoverCellWire = -1;   // 哪条线
    int  m_hoverCellIdx = -1;   // 哪一格
    wxPoint m_hoverCellPos;
    int HitHoverCell(const wxPoint& raw, int* wireIdx, int* cellIdx, wxPoint* cellPos);

	// 工具管理器需要的接口
public:
    void ClearSelection();
    void SetSelectedIndex(int index);
    int HitTestPublic(const wxPoint& pt);
    bool IsClickOnEmptyAreaPublic(const wxPoint& canvasPos);

    // 快捷工具栏
    QuickToolBar* m_quickToolBar;

    std::vector<WireWireAnchor> m_wireWireAnchors;// 导线<->导线小方块（新增）
    wxDECLARE_EVENT_TABLE();
};