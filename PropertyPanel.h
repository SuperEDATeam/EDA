#pragma once
#include <wx/wx.h>

class PropertyPanel : public wxPanel
{
public:
    PropertyPanel(wxWindow* parent);
    void ShowElement(const wxString& name);   // ����������
private:
    wxStaticText* m_title;   // ��ֻ��һ�����⣬�����ű�
};