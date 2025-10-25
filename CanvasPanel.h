#pragma once
#include <wx/wx.h>
#include <vector>
#include "CanvasElement.h"
#include "Wire.h"          // �� ��������������

/* �������϶�ʱ��Ҫ���µ��������� + ��Ӧ������Ϣ */
struct WireAnchor {
    size_t wireIdx;   // ������
    size_t ptIdx;     // ���ߵڼ������Ƶ㣨0 �� ���
    bool   isInput;   // Ԫ���������뻹���������
    size_t pinIdx;    // Ԫ������������
};


class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(wxWindow* parent);
    void AddElement(const CanvasElement& elem);
    void PlaceElement(const wxString& name, const wxPoint& pos);

    // ����¼�������ԭ�϶� + �������ߣ�
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);

    // ��¶�������������ⲿ����/����ʹ��
    const std::vector<Wire>& GetWires() const { return m_wires; }

private:
    /* ---------- ԭ��Ԫ����� ---------- */
    std::vector<CanvasElement> m_elements;
    int  m_selectedIndex = -1;
    bool m_isDragging = false;
    wxPoint m_dragStartPos;
    wxPoint m_elementStartPos;

    /* ---------- ����������� ---------- */
    std::vector<Wire> m_wires;   // �Ѷ��͵�����
    Wire m_tempWire;             // �����϶�/Ԥ��������
    enum class WireMode { Idle, DragNew, DragMove };
    WireMode m_wireMode = WireMode::Idle;

    ControlPoint m_startCP;      // �������
    wxPoint m_curSnap;           // ��ǰ����/Ԥ����

    /* ---------- ԭ�к��� ---------- */
    void OnPaint(wxPaintEvent& evt);
    int  HitTest(const wxPoint& pt);

    /* ---------- �������� ---------- */
    // ���������� + ����
    wxPoint Snap(const wxPoint& raw, bool* snapped);

    std::vector<WireAnchor> m_movingWires;

    wxDECLARE_EVENT_TABLE();
};