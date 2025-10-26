#pragma once
#include <wx/wx.h>
#include <vector>
#include "CanvasElement.h"

class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(wxWindow* parent);
    void AddElement(const CanvasElement& elem);
    void OnLeftDown(wxMouseEvent& evt);  // ��갴�£�ѡ��/��ʼ�϶���
    void OnLeftUp(wxMouseEvent& evt);    // ����ͷţ������϶���
    void OnMouseMove(wxMouseEvent& evt); // ����ƶ��������϶���
    void OnKeyDown(wxKeyEvent& evt);     // ����ɾ����
    void PlaceElement(const wxString& name, const wxPoint& pos);
private:
    std::vector<CanvasElement> m_elements;
    int m_selectedIndex = -1;  // ѡ�е�Ԫ��������-1��ʾδѡ�У�
    bool m_isDragging = false; // �Ƿ������϶�
    wxPoint m_dragStartPos;    // �϶���ʼʱ�����λ��
    wxPoint m_elementStartPos; // �϶���ʼʱ��Ԫ��λ��

    void OnPaint(wxPaintEvent& evt);
    // �����Ƿ���Ԫ����Χ�ڣ������ж�ѡ�У�
    int HitTest(const wxPoint& pt);
    wxDECLARE_EVENT_TABLE();
};