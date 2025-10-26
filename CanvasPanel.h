#pragma once
#include <wx/wx.h>
#include <vector>
#include "CanvasElement.h"

class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(wxWindow* parent);
    void AddElement(const CanvasElement& elem);
    void OnLeftDown(wxMouseEvent& evt);  // 鼠标按下（选中/开始拖动）
    void OnLeftUp(wxMouseEvent& evt);    // 鼠标释放（结束拖动）
    void OnMouseMove(wxMouseEvent& evt); // 鼠标移动（处理拖动）
    void OnKeyDown(wxKeyEvent& evt);     // 处理删除键
    void PlaceElement(const wxString& name, const wxPoint& pos);
private:
    std::vector<CanvasElement> m_elements;
    int m_selectedIndex = -1;  // 选中的元件索引（-1表示未选中）
    bool m_isDragging = false; // 是否正在拖动
    wxPoint m_dragStartPos;    // 拖动开始时的鼠标位置
    wxPoint m_elementStartPos; // 拖动开始时的元件位置

    void OnPaint(wxPaintEvent& evt);
    // 检测点是否在元件范围内（用于判断选中）
    int HitTest(const wxPoint& pt);
    wxDECLARE_EVENT_TABLE();
};