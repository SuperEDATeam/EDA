#pragma once
#include <wx/wx.h>
#include <wx/treectrl.h>

class ToolboxPanel : public wxPanel
{
public:
    explicit ToolboxPanel(wxWindow* parent);
    ~ToolboxPanel();
    void Rebuild();                 // 外部调用，重建树
    void OnSelectionChanged(wxTreeEvent& evt);
    void OnMouseDown(wxMouseEvent& evt);
private:
    wxTreeCtrl* m_tree;
    wxImageList* m_imgList;      // 图标池
    wxTreeItemId m_selectedItem;    // 记录当前选中项（可选）
    wxString     m_itemName;        // 记录选中名称
    void OnItemActivated(wxTreeEvent& evt);
};