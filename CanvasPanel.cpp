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

//================= ���� =================
CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxYELLOW);
    MyLog("CanvasPanel: constructed\n");
}

//================= ���Ԫ�� =================
void CanvasPanel::AddElement(const CanvasElement& elem)
{
    m_elements.push_back(elem);
    Refresh();
    MyLog("CanvasPanel::AddElement: <%s> total=%zu\n",
        elem.GetName().ToUTF8().data(), m_elements.size());
}

//================= ���� =================
void CanvasPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    // 1. ����
    const int grid = 20;
    const wxColour c(240, 240, 240);
    dc.SetPen(wxPen(c, 1));
    wxSize sz = GetClientSize();
    for (int x = 0; x < sz.x; x += grid)
        dc.DrawLine(x, 0, x, sz.y);
    for (int y = 0; y < sz.y; y += grid)
        dc.DrawLine(0, y, sz.x, y);

    // 2. ����Ԫ��
    for (size_t i = 0; i < m_elements.size(); ++i) {
        m_elements[i].Draw(dc);
        // ѡ�и���
        if ((int)i == m_selectedIndex) {
            wxRect b = m_elements[i].GetBounds();
            dc.SetPen(wxPen(*wxRED, 2, wxPENSTYLE_DOT));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(b);
        }
    }

    // 3. ���ߣ����� + Ԥ����
    for (const auto& w : m_wires) w.Draw(dc);
    if (m_wireMode == WireMode::DragNew) m_tempWire.Draw(dc);
}

//================= ��갴�� =================
void CanvasPanel::OnLeftDown(wxMouseEvent& evt)
{
    wxPoint raw = evt.GetPosition();
    bool snapped = false;
    wxPoint pos = Snap(raw, &snapped);

    // ---- �����߼����������ſ�ʼ���� ----
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

    //    // �ؼ���ֻҪ���������ţ��Ͱ����˶����Ϊ Pin���Ա�˫���ռ�
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
        wxPoint endPos = Snap(pos, &snappedEnd);   // �� Snap ��֤�������Ż�����

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

    // ---- ԭ��Ԫ���϶�/�����߼� ----
    m_selectedIndex = HitTest(raw);
    if (m_selectedIndex != -1) {
        m_isDragging = true;
        m_dragStartPos = raw;
        m_elementStartPos = m_elements[m_selectedIndex].GetPos();

        // ===== ˫���ռ������ OR �յ���������Ԫ������ =====
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

                    // �������
                    if (wire.pts.front().type == CPType::Pin &&
                        wire.pts.front().pos == pinWorld) {
                        m_movingWires.push_back({ w, 0, isInput, p });
                        MyLog("      HIT START\n");
                        MyLog("Collect START wire=%zu pt=0 pin=%zu\n", w, p);
                    }
                    // �յ�����
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

//================= ����ƶ� =================
void CanvasPanel::OnMouseMove(wxMouseEvent& evt)
{
    // ---- ����Ԥ�� ----
    if (m_wireMode == WireMode::DragNew) {
        bool snapped = false;
        m_curSnap = Snap(evt.GetPosition(), &snapped);
        auto route = Wire::RouteOrtho(m_startCP.pos, m_curSnap);
        m_tempWire.pts = route;
        Refresh();
        return;
    }

    // ---- Ԫ���϶� + ����ʵʱ��·�ɣ�Manhattan�� ----
    if (m_isDragging && m_selectedIndex != -1) {
        wxPoint delta = evt.GetPosition() - m_dragStartPos;
        m_elements[m_selectedIndex].SetPos(m_elementStartPos + delta);

        // ��������������Ԫ�����ŵ��߶Σ������������ɺ�-��-��
        for (const auto& aw : m_movingWires) {
            Wire& wire = m_wires[aw.wireIdx];

            // ���¼�����������
            wxPoint newPin = m_elements[m_selectedIndex].GetPos() +
                wxPoint((aw.isInput ?
                    m_elements[m_selectedIndex].GetInputPins()[aw.pinIdx].pos :
                    m_elements[m_selectedIndex].GetOutputPins()[aw.pinIdx].pos).x,
                    (aw.isInput ?
                        m_elements[m_selectedIndex].GetInputPins()[aw.pinIdx].pos :
                        m_elements[m_selectedIndex].GetOutputPins()[aw.pinIdx].pos).y);

            wxPoint startPt = (aw.ptIdx == 0) ? newPin : wire.pts.front().pos;
            wxPoint endPt = (aw.ptIdx == wire.pts.size() - 1) ? newPin : wire.pts.back().pos;

            // �������۵㣬ȫ�̺�-��-��
            wire.pts = Wire::RouteOrtho(startPt, endPt);
        }

        Refresh();
    }
}

//================= ����ͷ� =================
void CanvasPanel::OnLeftUp(wxMouseEvent& evt)
{
    m_isDragging = false;
    m_movingWires.clear();   // �϶������������
}

//================= ���� =================
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

//================= ����Ԫ�� =================
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

//================= ���в��� =================
int CanvasPanel::HitTest(const wxPoint& pt)
{
    for (size_t i = 0; i < m_elements.size(); ++i)
        if (m_elements[i].GetBounds().Contains(pt)) return i;
    return -1;
}

//================= ����������+���� =================
wxPoint CanvasPanel::Snap(const wxPoint& raw, bool* snapped)
{
    *snapped = false;
    const int grid = 20;
    wxPoint s((raw.x + grid / 2) / grid * grid, (raw.y + grid / 2) / grid * grid);

    // �����ţ��뾶 8 px��
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