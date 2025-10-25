#pragma once
#include <wx/wx.h>
#include <vector>
#include "CanvasElement.h"
#include "Wire.h"          // ← 新增：连线数据

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

    // 新增：缩放因子（默认1.0，即100%）
    float m_scale = 1.0f;

    /* ---------- 原有函数 ---------- */
    void OnPaint(wxPaintEvent& evt);
    int  HitTest(const wxPoint& pt);

    /* ---------- 新增辅助 ---------- */
    // 吸附：网格 + 引脚
    wxPoint Snap(const wxPoint& raw, bool* snapped);

    std::vector<WireAnchor> m_movingWires;

    wxDECLARE_EVENT_TABLE();
};