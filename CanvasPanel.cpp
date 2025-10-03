#include "CanvasPanel.h"
#include <wx/dcbuffer.h>   // 双缓冲
#include "CanvasElement.h" // 新增
#include "CanvasDropTarget.h"
#include "my_log.h"

wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
wxEND_EVENT_TABLE()

CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); // 双缓冲必须
    SetDropTarget(new CanvasDropTarget(this));
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxYELLOW);
    MyLog("CanvasPanel: drop target set\n");
}

void CanvasPanel::AddElement(const CanvasElement& elem)
{
    m_elements.push_back(elem);
    Refresh();
    MyLog("CanvasPanel::AddElement: <%s> total=%zu\n",
        elem.GetName().ToUTF8().data(), m_elements.size());
}

void CanvasPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    //dc.SetBackground(*wxYELLOW);
    dc.Clear();

    // 1. 画网格（你原来就有）
    const int grid = 20;
    const wxColour c(240, 240, 240);
    dc.SetPen(wxPen(c, 1));
    wxSize sz = GetClientSize();
    for (int x = 0; x < sz.x; x += grid) {
        dc.DrawLine(x - 2, 0, x + 2, sz.y);   // 垂直贯穿
        dc.DrawLine(0, x - 2, sz.x, x + 2);   // 水平贯穿
    }

    // 2. 画所有元件（新增）
    for (const auto& elem : m_elements) {
        elem.Draw(dc);   // 调用 CanvasElement::Draw
    }
}