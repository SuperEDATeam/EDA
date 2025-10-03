#pragma once
#include <wx/wx.h>
#include <vector>
#include "CanvasElement.h"

class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(wxWindow* parent);
    void AddElement(const CanvasElement& elem);   // 挂元件
private:
    std::vector<CanvasElement> m_elements;   // 数据层
    void OnPaint(wxPaintEvent& evt);
    wxDECLARE_EVENT_TABLE();
};
