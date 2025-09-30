#include "PropertyPanel.h"

PropertyPanel::PropertyPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_title = new wxStaticText(this, wxID_ANY, "No element selected");
    sizer->Add(m_title, 0, wxALL, 5);
    SetSizer(sizer);
}

void PropertyPanel::ShowElement(const wxString& name)
{
    m_title->SetLabel("Tool: " + name);   // 先只改标题
    // 后续在这里动态插表单
}