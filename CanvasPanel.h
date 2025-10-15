#pragma once
#include <wx/wx.h>
#include <vector>
#include "CanvasElement.h"

class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(wxWindow* parent);
    void AddElement(const CanvasElement& elem);   // 挂元件
    void OnLeftDown(wxMouseEvent& evt);  // 捕获单击
    void OnKeyDown(wxKeyEvent& evt);     // ESC 取消放置
    void PlaceElement(const wxString& name, const wxPoint& pos); // 由 MainFrame 调用
private:
    std::vector<CanvasElement> m_elements;   // 数据层
    void OnPaint(wxPaintEvent& evt);
    wxDECLARE_EVENT_TABLE();
};
