#pragma once
#include <wx/wx.h>
#include <vector>
#include "CanvasElement.h"
#include "Wire.h"          // ← 新增：连线数据

class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(wxWindow* parent);
    void AddElement(const CanvasElement& elem);
    void PlaceElement(const wxString& name, const wxPoint& pos);

    // 鼠标事件（兼容原拖动 + 新增连线）
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);

    // 暴露连线容器，供外部仿真/导出使用
    const std::vector<Wire>& GetWires() const { return m_wires; }

private:
    /* ---------- 原有元件相关 ---------- */
    std::vector<CanvasElement> m_elements;
    int  m_selectedIndex = -1;
    bool m_isDragging = false;
    wxPoint m_dragStartPos;
    wxPoint m_elementStartPos;

    /* ---------- 新增连线相关 ---------- */
    std::vector<Wire> m_wires;   // 已定型的连线
    Wire m_tempWire;             // 正在拖动/预览的连线
    enum class WireMode { Idle, DragNew, DragMove };
    WireMode m_wireMode = WireMode::Idle;

    ControlPoint m_startCP;      // 连线起点
    wxPoint m_curSnap;           // 当前吸附/预览点

    /* ---------- 原有函数 ---------- */
    void OnPaint(wxPaintEvent& evt);
    int  HitTest(const wxPoint& pt);

    /* ---------- 新增辅助 ---------- */
    // 吸附：网格 + 引脚
    wxPoint Snap(const wxPoint& raw, bool* snapped);

    wxDECLARE_EVENT_TABLE();
};