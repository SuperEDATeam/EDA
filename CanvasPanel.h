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


    // ������ط���
    float GetScale() const { return m_scale; }  // ��ȡ��ǰ���ű���
    void SetScale(float scale);                 // �������ű�����ˢ��

    // ����ת������Ļ���� <-> �������꣬�������ţ�
    wxPoint ScreenToCanvas(const wxPoint& screenPos) const;
    wxPoint CanvasToScreen(const wxPoint& canvasPos) const;

    wxPoint m_offset;          // ����ƫ������ƽ�����꣩
    bool m_isPanning;          // �Ƿ�������קƽ��
    wxPoint m_panStartPos;     // ƽ�ƿ�ʼʱ����Ļ����


      // �������ж��Ƿ����ڿհ�������Ԫ�أ�
    bool IsClickOnEmptyArea(const wxPoint& canvasPos);

    // �¼������������м���Ϊ�����أ�
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt);  
   

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

    // �������������ӣ�Ĭ��1.0����100%��
    float m_scale = 1.0f;

    /* ---------- ԭ�к��� ---------- */
    void OnPaint(wxPaintEvent& evt);
    int  HitTest(const wxPoint& pt);

    /* ---------- �������� ---------- */
    // ���������� + ����
    wxPoint Snap(const wxPoint& raw, bool* snapped);

    std::vector<WireAnchor> m_movingWires;

    int m_hoverPinIdx=-1;
    bool m_hoverIsInput=false;
    wxPoint m_hoverPinPos;
    int  HitHoverPin(const wxPoint& raw, bool* isInput, wxPoint* worldPos);

    int  m_hoverCellWire = -1;   // ������
    int  m_hoverCellIdx = -1;   // ��һ��
    wxPoint m_hoverCellPos;
    int HitHoverCell(const wxPoint& raw, int* wireIdx, int* cellIdx, wxPoint* cellPos);

    std::vector<WireWireAnchor> m_wireWireAnchors;// ����<->����С���飨������
    wxDECLARE_EVENT_TABLE();
};