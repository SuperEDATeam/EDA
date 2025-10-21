#include "CanvasPanel.h"
#include "MainFrame.h"
#include "Wire.h"            // ← 新增
#include <wx/dcbuffer.h>
#include "CanvasElement.h"
#include "my_log.h"

wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)
EVT_LEFT_UP(CanvasPanel::OnLeftUp)
EVT_MOTION(CanvasPanel::OnMouseMove)
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
wxEND_EVENT_TABLE()

//================= 构造 =================
CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxYELLOW);
    MyLog("CanvasPanel: constructed\n");
}

//================= 添加元件 =================
void CanvasPanel::AddElement(const CanvasElement& elem)
{
    m_elements.push_back(elem);
    Refresh();
    MyLog("CanvasPanel::AddElement: <%s> total=%zu\n",
        elem.GetName().ToUTF8().data(), m_elements.size());
}

//================= 绘制 =================
void CanvasPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    // 1. 网格
    const int grid = 20;
    const wxColour c(240, 240, 240);
    dc.SetPen(wxPen(c, 1));
    wxSize sz = GetClientSize();
    for (int x = 0; x < sz.x; x += grid)
        dc.DrawLine(x, 0, x, sz.y);
    for (int y = 0; y < sz.y; y += grid)
        dc.DrawLine(0, y, sz.x, y);

    // 2. 所有元件
    for (size_t i = 0; i < m_elements.size(); ++i) {
        m_elements[i].Draw(dc);
        // 选中高亮
        if ((int)i == m_selectedIndex) {
            wxRect b = m_elements[i].GetBounds();
            dc.SetPen(wxPen(*wxRED, 2, wxPENSTYLE_DOT));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(b);
        }
    }

    // 3. 连线（定型 + 预览）
    for (const auto& w : m_wires) w.Draw(dc);
    if (m_wireMode == WireMode::DragNew) m_tempWire.Draw(dc);
}

//================= 鼠标按下 =================
void CanvasPanel::OnLeftDown(wxMouseEvent& evt)
{
    wxPoint raw = evt.GetPosition();
    bool snapped = false;
    wxPoint pos = Snap(raw, &snapped);

    // ---- 连线逻辑：单击引脚开始拖线 ----
    if (m_wireMode == WireMode::Idle && snapped) {
        m_startCP = { pos, CPType::Pin };
        m_tempWire.Clear();
        m_tempWire.AddPoint(m_startCP);
        m_wireMode = WireMode::DragNew;
        CaptureMouse();
        Refresh();
        return;
    }
    if (m_wireMode == WireMode::DragNew) {
        ControlPoint end{ pos, snapped ? CPType::Pin : CPType::Free };
        m_tempWire.AddPoint(end);
        m_wires.emplace_back(m_tempWire);   // 定型
        m_wireMode = WireMode::Idle;
        ReleaseMouse();
        Refresh();
        return;
    }

    // ---- 原有元件拖动/放置逻辑 ----
    m_selectedIndex = HitTest(raw);
    if (m_selectedIndex != -1) {
        m_isDragging = true;
        m_dragStartPos = raw;
        m_elementStartPos = m_elements[m_selectedIndex].GetPos();
        SetFocus();
    }
    else {
        m_selectedIndex = -1;
        m_isDragging = false;
        MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
        if (mf && !mf->GetPendingTool().IsEmpty()) {
            PlaceElement(mf->GetPendingTool(), pos);
            mf->ClearPendingTool();
        }
    }
    Refresh();
}

//================= 鼠标移动 =================
void CanvasPanel::OnMouseMove(wxMouseEvent& evt)
{
    // ---- 连线预览 ----
    if (m_wireMode == WireMode::DragNew) {
        bool snapped = false;
        m_curSnap = Snap(evt.GetPosition(), &snapped);
        auto route = Wire::RouteOrtho(m_startCP.pos, m_curSnap);
        m_tempWire.pts = route;
        Refresh();
        return;
    }

    // ---- 原有元件拖动 ----
    if (m_isDragging && m_selectedIndex != -1) {
        wxPoint delta = evt.GetPosition() - m_dragStartPos;
        m_elements[m_selectedIndex].SetPos(m_elementStartPos + delta);
        Refresh();
    }
}

//================= 鼠标释放 =================
void CanvasPanel::OnLeftUp(wxMouseEvent& evt)
{
    m_isDragging = false;
}

//================= 键盘 =================
void CanvasPanel::OnKeyDown(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_ESCAPE) {
        MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
        if (mf) mf->ClearPendingTool();
    }
    else if (evt.GetKeyCode() == WXK_DELETE && m_selectedIndex != -1) {
        m_elements.erase(m_elements.begin() + m_selectedIndex);
        m_selectedIndex = -1;
        Refresh();
    }
    else {
        evt.Skip();
    }
}

//================= 放置元件 =================
void CanvasPanel::PlaceElement(const wxString& name, const wxPoint& pos)
{
    extern std::vector<CanvasElement> g_elements;
    auto it = std::find_if(g_elements.begin(), g_elements.end(),
        [&](const CanvasElement& e) { return e.GetName() == name; });
    if (it == g_elements.end()) return;
    CanvasElement clone = *it;
    clone.SetPos(pos);
    AddElement(clone);
}

//================= 命中测试 =================
int CanvasPanel::HitTest(const wxPoint& pt)
{
    for (size_t i = 0; i < m_elements.size(); ++i)
        if (m_elements[i].GetBounds().Contains(pt)) return i;
    return -1;
}

//================= 吸附：网格+引脚 =================
wxPoint CanvasPanel::Snap(const wxPoint& raw, bool* snapped)
{
    *snapped = false;
    const int grid = 20;
    wxPoint s((raw.x + grid / 2) / grid * grid, (raw.y + grid / 2) / grid * grid);

    // 吸引脚（半径 8 px）
    for (const auto& elem : m_elements) {
        auto testPins = [&](const auto& pins) {
            for (const auto& pin : pins) {
                wxPoint p = elem.GetPos() + wxPoint(pin.pos.x, pin.pos.y);
                if (abs(raw.x - p.x) <= 8 && abs(raw.y - p.y) <= 8) {
                    *snapped = true;
                    return p;
                }
            }
            return wxPoint{};
            };
        wxPoint in = testPins(elem.GetInputPins());
        if (in != wxPoint{}) return in;
        wxPoint out = testPins(elem.GetOutputPins());
        if (out != wxPoint{}) return out;
    }
    return s;
}