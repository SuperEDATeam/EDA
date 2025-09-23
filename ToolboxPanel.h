#pragma once
#include <wx/wx.h>
#include <wx/treectrl.h>

class ToolboxPanel : public wxPanel
{
public:
    explicit ToolboxPanel(wxWindow* parent);
    void Rebuild();                 // 外部调用，重建树
private:
    wxTreeCtrl* m_tree;
    wxImageList* m_imgList;      // 图标池
    void OnItemActivated(wxTreeEvent& evt);
    void OnBeginDrag(wxTreeEvent& evt);
    wxDECLARE_EVENT_TABLE();
};