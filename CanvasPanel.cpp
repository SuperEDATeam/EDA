#include "CanvasPanel.h"
#include "MainFrame.h"
#include "Wire.h"
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
    //if (m_wireMode == WireMode::DragNew) {
    //    ControlPoint end{ pos, snapped ? CPType::Pin : CPType::Free };
    //    m_tempWire.AddPoint(end);

    //    // 关键：只要吸附到引脚，就把两端都标记为 Pin，以便双向收集
    //    if (snapped) {
    //        m_tempWire.pts.front().type = CPType::Pin;
    //        MyLog("Before: back.type=%d\n", int(m_tempWire.pts.back().type));
    //        m_tempWire.pts.back().type = CPType::Pin;
    //        MyLog("After : back.type=%d\n", int(m_tempWire.pts.back().type));
    //    }

    //    MyLog("After set: wire front type=%d  back type=%d\n",
    //        int(m_tempWire.pts.front().type),
    //        int(m_tempWire.pts.back().type));

    //    MyLog("Assign: &front=%p  &back=%p  size=%zu\n",
    //        &m_tempWire.pts.front().type,
    //        &m_tempWire.pts.back().type,
    //        m_tempWire.pts.size());
    //    m_wires.emplace_back(m_tempWire);
    //    m_wireMode = WireMode::Idle;
    //    ReleaseMouse();
    //    Refresh();
    //    return;
    //}
    if (m_wireMode == WireMode::DragNew) {
        bool snappedEnd = false;
        wxPoint endPos = Snap(pos, &snappedEnd);   // 用 Snap 保证落在引脚或网格

        ControlPoint end{ endPos, snappedEnd ? CPType::Pin : CPType::Free };
        m_tempWire.AddPoint(end);

        if (snappedEnd) {
            m_tempWire.pts.front().type = CPType::Pin;
            m_tempWire.pts.back().type = CPType::Pin;
        }

        m_wires.emplace_back(m_tempWire);
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

        // ===== 双向收集：起点 OR 终点连到被拖元件引脚 =====
        m_movingWires.clear();
        const CanvasElement& elem = m_elements[m_selectedIndex];
        MyLog("Drag elem POS = (%d,%d)\n", elem.GetPos().x, elem.GetPos().y);
        auto collect = [&](const auto& pins, bool isInput) {
            for (size_t p = 0; p < pins.size(); ++p) {
                wxPoint pinWorld = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
                MyLog("  pin[%zu] world = (%d,%d)\n", p, pinWorld.x, pinWorld.y);
                for (size_t w = 0; w < m_wires.size(); ++w) {
                    const auto& wire = m_wires[w];

                    MyLog("    wire[%zu] front=(%d,%d) type=%d  back=(%d,%d) type=%d\n",
                        w,
                        wire.pts.front().pos.x, wire.pts.front().pos.y, int(wire.pts.front().type),
                        wire.pts.back().pos.x, wire.pts.back().pos.y, int(wire.pts.back().type));

                    // 起点命中
                    if (wire.pts.front().type == CPType::Pin &&
                        wire.pts.front().pos == pinWorld) {
                        m_movingWires.push_back({ w, 0, isInput, p });
                        MyLog("      HIT START\n");
                        MyLog("Collect START wire=%zu pt=0 pin=%zu\n", w, p);
                    }
                    // 终点命中
                    if (wire.pts.size() > 1 &&
                        wire.pts.back().type == CPType::Pin &&
                        wire.pts.back().pos == pinWorld) {
                        m_movingWires.push_back({ w, wire.pts.size() - 1, isInput, p });
                        MyLog("      HIT END\n");
                        MyLog("Collect END   wire=%zu pt=%zu pin=%zu\n", w, wire.pts.size() - 1, p);
                    }
                }
            }
            };
        collect(elem.GetInputPins(), true);
        collect(elem.GetOutputPins(), false);
        MyLog("Collect total=%zu\n", m_movingWires.size());
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

    // ---- 元件拖动 + 连线实时重路由（Manhattan） ----
    if (m_isDragging && m_selectedIndex != -1) {
        wxPoint delta = evt.GetPosition() - m_dragStartPos;
        m_elements[m_selectedIndex].SetPos(m_elementStartPos + delta);

        // 对所有连到被拖元件引脚的线段：整条重新生成横-竖-横
        for (const auto& aw : m_movingWires) {
            Wire& wire = m_wires[aw.wireIdx];

            // 重新计算两端坐标
            wxPoint newPin = m_elements[m_selectedIndex].GetPos() +
                wxPoint((aw.isInput ?
                    m_elements[m_selectedIndex].GetInputPins()[aw.pinIdx].pos :
                    m_elements[m_selectedIndex].GetOutputPins()[aw.pinIdx].pos).x,
                    (aw.isInput ?
                        m_elements[m_selectedIndex].GetInputPins()[aw.pinIdx].pos :
                        m_elements[m_selectedIndex].GetOutputPins()[aw.pinIdx].pos).y);

            wxPoint startPt = (aw.ptIdx == 0) ? newPin : wire.pts.front().pos;
            wxPoint endPt = (aw.ptIdx == wire.pts.size() - 1) ? newPin : wire.pts.back().pos;

            // 丢弃旧折点，全程横-竖-横
            wire.pts = Wire::RouteOrtho(startPt, endPt);
        }

        Refresh();
    }
}

//================= 鼠标释放 =================
void CanvasPanel::OnLeftUp(wxMouseEvent& evt)
{
    m_isDragging = false;
    m_movingWires.clear();   // 拖动结束即可清空
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