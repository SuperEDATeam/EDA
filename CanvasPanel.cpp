#include "CanvasPanel.h"
#include "MainFrame.h"
#include <wx/dcbuffer.h>   // 双缓冲
#include "CanvasElement.h" // 新增
#include "my_log.h"

wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
wxEND_EVENT_TABLE()

CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); // 双缓冲必须
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

void CanvasPanel::OnLeftDown(wxMouseEvent& evt)
{
    wxPoint pt = evt.GetPosition();
    MyLog("CanvasPanel: mouse %d,%d\n", pt.x, pt.y);

    MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
    if (!mf) return;
    const wxString& tool = mf->GetPendingTool();
    if (tool.IsEmpty()) return;          // 普通单击，忽略
    PlaceElement(tool, evt.GetPosition());
    mf->ClearPendingTool();              // 一次放置后自动退出“放置模式”
}

void CanvasPanel::OnKeyDown(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_ESCAPE)
    {
        MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
        if (mf) mf->ClearPendingTool();
    }
    else
        evt.Skip();
}

void CanvasPanel::PlaceElement(const wxString& name, const wxPoint& pos)
{
    extern std::vector<CanvasElement> g_elements;
    auto it = std::find_if(g_elements.begin(), g_elements.end(),
        [&](const CanvasElement& e) { return e.GetName() == name; });
    if (it == g_elements.end()) return;
    CanvasElement clone = *it;
    clone.SetPos(pos);
    AddElement(clone);          // 复用你已有的 AddElement/Refresh
}