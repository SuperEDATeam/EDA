#pragma once
#include <wx/wx.h>

class PropertyPanel : public wxPanel
{
public:
    PropertyPanel(wxWindow* parent);
    void ShowElement(const wxString& name);   // 后续填表入口
private:
    wxStaticText* m_title;   // 先只放一个标题，后续放表单
};