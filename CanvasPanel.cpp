#include "CanvasPanel.h"
#include "MainFrame.h"
#include "Wire.h"            // �� ����
#include <wx/dcbuffer.h>
#include "CanvasElement.h"
#include "my_log.h"

wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)
EVT_LEFT_UP(CanvasPanel::OnLeftUp)
EVT_MOTION(CanvasPanel::OnMouseMove)
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
EVT_MOUSEWHEEL(CanvasPanel::OnMouseWheel)  // �������󶨹����¼�
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)    // ������£����ڿ�ʼƽ�ƻ����Ԫ�أ�
EVT_LEFT_UP(CanvasPanel::OnLeftUp)        // ����ͷţ�����ƽ�ƻ����Ԫ�أ�
EVT_MOTION(CanvasPanel::OnMouseMove)      // ����ƶ�������ƽ�ƻ�Ԫ����ק��
wxEND_EVENT_TABLE()


//================= ���� =================
CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE),
    m_offset(0, 0),  // ��ʼ��ƫ����
    m_isPanning(false)  // ��ʼ��ƽ��״̬
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxYELLOW);
    MyLog("CanvasPanel: constructed\n");
}


bool CanvasPanel::IsClickOnEmptyArea(const wxPoint& canvasPos)
{
    // ��������Ԫ�أ��жϵ��λ���Ƿ����κ�Ԫ���ڲ�
    for (const auto& elem : m_elements) {
        if (elem.GetBounds().Contains(canvasPos)) {
            return false; // �����Ԫ���ϣ����ǿհ�����
        }
    }
    return true; // �հ�����
}  

// �������������ű��������Ʒ�Χ0.1~5.0������������ţ�
void CanvasPanel::SetScale(float scale)
{
    if (scale < 0.1f) scale = 0.1f;
    if (scale > 5.0f) scale = 5.0f;
    m_scale = scale;
    Refresh();  // �����ػ�
    // ����״̬����ʾ���ű����������Ҫ��
    MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
    if (mf) {
        mf->SetStatusText(wxString::Format("Zoom: %.0f%%", m_scale * 100));
    }
}


// ��������Ļ����ת�������꣨�����������ӣ�
wxPoint CanvasPanel::ScreenToCanvas(const wxPoint& screenPos) const
{
    return wxPoint(
        static_cast<int>((screenPos.x - m_offset.x) / m_scale),
        static_cast<int>((screenPos.y - m_offset.y) / m_scale)
    );
}

// ��������������ת��Ļ���꣨�����������ӣ�
wxPoint CanvasPanel::CanvasToScreen(const wxPoint& canvasPos) const
{
    return wxPoint(
        static_cast<int>(canvasPos.x * m_scale + m_offset.x),
        static_cast<int>(canvasPos.y * m_scale + m_offset.y)
    );
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
// �޸ģ���ͼʱӦ����������
void CanvasPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    // Ӧ�����ź�ƫ��
    dc.SetUserScale(m_scale, m_scale);
    dc.SetDeviceOrigin(m_offset.x, m_offset.y);  // �����豸ԭ��ƫ��

    // 1. �������������С����������Ӧ��
    const int grid = 20;
    const wxColour c(240, 240, 240);
    dc.SetPen(wxPen(c, 1));
    // ����ɼ����������Χ���������ź�Ļ�����С��
    wxSize sz = GetClientSize();
    int maxX = static_cast<int>(sz.x / m_scale);  // ת��Ϊ��������
    int maxY = static_cast<int>(sz.y / m_scale);
    for (int x = 0; x < maxX; x += grid)
        dc.DrawLine(x, 0, x, maxY);
    for (int y = 0; y < maxY; y += grid)
        dc.DrawLine(0, y, maxX, y);

    // 2. ����Ԫ�أ�Ԫ����������CanvasElement�ڲ�ά����������DC�Զ�����
    for (size_t i = 0; i < m_elements.size(); ++i) {
        m_elements[i].Draw(dc);
        // ѡ��״̬�߿�
        if ((int)i == m_selectedIndex) {
            wxRect b = m_elements[i].GetBounds();
            dc.SetPen(wxPen(*wxRED, 2, wxPENSTYLE_DOT));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(b);
        }
    }

    // 3. ���Ƶ��ߣ�����������ڻ�����������DC����
    for (const auto& w : m_wires) w.Draw(dc);
    if (m_wireMode == WireMode::DragNew) m_tempWire.Draw(dc);
}

void CanvasPanel::OnLeftDown(wxMouseEvent& evt)
{
    wxPoint rawScreenPos = evt.GetPosition();
    wxPoint rawCanvasPos = ScreenToCanvas(rawScreenPos);
    wxPoint canvasPos = rawCanvasPos;

    // �ȴ����߻���ģʽ
    bool snapped = false;
    wxPoint pos = Snap(rawCanvasPos, &snapped);

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
        m_wires.emplace_back(m_tempWire);
        m_wireMode = WireMode::Idle;
        ReleaseMouse();
        Refresh();
        return;
    }

    // ����Ԫ��ѡ����϶�
    m_selectedIndex = HitTest(rawCanvasPos);
    if (m_selectedIndex != -1) {
        m_isDragging = true;
        m_dragStartPos = rawScreenPos;
        m_elementStartPos = m_elements[m_selectedIndex].GetPos();
        SetFocus();
        Refresh();
        return;
    }

    // ����Ԫ�����ã�������ƽ�ƣ�
    MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
    if (mf && !mf->GetPendingTool().IsEmpty()) {
        PlaceElement(mf->GetPendingTool(), pos);
        mf->ClearPendingTool();
        Refresh();
        return;
    }

    // �����հ������ƽ��
    if (IsClickOnEmptyArea(canvasPos)) {
        m_isPanning = true;
        m_panStartPos = rawScreenPos;
        SetCursor(wxCURSOR_HAND);
        CaptureMouse();
        return;
    }

    Refresh();
}






// �޸ģ�����ƶ��¼��У��������ź������
void CanvasPanel::OnMouseMove(wxMouseEvent& evt)
{

    // ���ȴ���ƽ��
    if (m_isPanning) {
        wxPoint delta = evt.GetPosition() - m_panStartPos; // ������Ļ����ƫ��
        m_offset += delta; // ���»���ƫ������ƽ�ƺ��ģ�
        m_panStartPos = evt.GetPosition(); // �������
        Refresh(); // �ػ滭��
        return;
    }
    // ƽ�ƴ���
    if (m_isPanning) {
        wxPoint delta = evt.GetPosition() - m_panStartPos;
        m_offset += delta;
        m_panStartPos = evt.GetPosition();
        Refresh();
        return;
    }
    if (m_wireMode == WireMode::DragNew) {
        // ����Ԥ�������λ��ת��Ϊ��������
        wxPoint rawCanvasPos = ScreenToCanvas(evt.GetPosition());
        bool snapped = false;
        m_curSnap = Snap(rawCanvasPos, &snapped);
        auto route = Wire::RouteOrtho(m_startCP.pos, m_curSnap);
        m_tempWire.pts = route;
        Refresh();
        return;
    }

    if (m_isDragging && m_selectedIndex != -1) {
        // Ԫ���϶���delta������Ļ������㣬ת��Ϊ����������ƶ���
        wxPoint deltaScreen = evt.GetPosition() - m_dragStartPos;
        // ��Ļ�����delta�����������ӣ��õ�����������ƶ���
        wxPoint deltaCanvas(
            static_cast<int>(deltaScreen.x / m_scale),
            static_cast<int>(deltaScreen.y / m_scale)
        );
        m_elements[m_selectedIndex].SetPos(m_elementStartPos + deltaCanvas);
        Refresh();
    }
}

//================= ����ͷ� =================
void CanvasPanel::OnLeftUp(wxMouseEvent& evt)
{
    if (m_isPanning) {
        // ����ƽ��
        m_isPanning = false;
        ReleaseMouse();
        SetCursor(wxCURSOR_ARROW); // �ָ���ͷ���
        return;
    }
    m_isDragging = false;
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
// �޸ģ�HitTestʹ�û��������ж�
int CanvasPanel::HitTest(const wxPoint& pt)  // pt��ת��Ϊ��������
{
    for (size_t i = 0; i < m_elements.size(); ++i) {
        // Ԫ�صı߽��ǻ������ֱ꣬�ӱȽ�
        if (m_elements[i].GetBounds().Contains(pt)) {
            return i;
        }
    }
    return -1;
}
// �޸��������¼�������Ctrl������ʱ����
void CanvasPanel::OnMouseWheel(wxMouseEvent& evt)
{
    // ֻ�а�סCtrl��ʱ�Ŵ�������
    if (!evt.ControlDown()) {
        evt.Skip();
        return;
    }

    // ��ȡ����ڻ����ϵ�λ�ã������������ļ��㣩
    wxPoint mouseScreenPos = evt.GetPosition();
    wxPoint mouseCanvasPos = ScreenToCanvas(mouseScreenPos);

    // ���浱ǰ���ű���
    float oldScale = m_scale;

    // ���ݹ��ַ����������
    if (evt.GetWheelRotation() > 0) {
        SetScale(m_scale * 1.2f);  // �Ŵ�
    }
    else {
        SetScale(m_scale / 1.2f);  // ��С
    }

    // ����ƫ������ʹ���ָ��Ļ���λ�ñ��ֲ���
    wxPoint newMouseScreenPos = CanvasToScreen(mouseCanvasPos);
    m_offset += mouseScreenPos - newMouseScreenPos;

    evt.Skip(false);  // ���ٴ����¼�
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