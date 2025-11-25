#pragma once
#include <wx/wx.h>
#include <wx/treectrl.h>
#include <map>
#include <vector>

class ToolboxPanel : public wxPanel
{
public:
    explicit ToolboxPanel(wxWindow* parent);
    void Rebuild();                 // 外部调用，重建工具树

    // 添加公共访问方法
    wxTreeCtrl* GetTree() const { return m_tree; }

private:
    wxTreeCtrl* m_tree;
    wxImageList* m_imgList;

    // 原有函数声明
    void LoadToolIcon(const wxString& toolName, const wxString& pngFileName);
    int GetToolIconIndex(const wxString& toolName);
    void OnItemActivated(wxTreeEvent& evt);
    void OnBeginDrag(wxTreeEvent& evt);
    void OnToolSelected(wxTreeEvent& evt);

    wxDECLARE_EVENT_TABLE();
};