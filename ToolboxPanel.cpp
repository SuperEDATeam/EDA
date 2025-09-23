#include "ToolboxPanel.h"
#include "ToolboxModel.h"
#include <wx/artprov.h>
#include <wx/dnd.h>
#include <wx/treectrl.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>        
#include <wx/file.h>

static void MY_LOG(const wxString& s)
{
    wxFile f(wxStandardPaths::Get().GetTempDir() + "\\logsim_tools.log",
        wxFile::write_append);
    if (f.IsOpened()) {
        f.Write(wxDateTime::Now().FormatISOCombined() + "  " + s + "\n");
        f.Close();
    }
}

// 轻量级数据节点
class wxStringTreeItemData : public wxTreeItemData
{
public:
    explicit wxStringTreeItemData(const wxString& s) : m_str(s) {}
    const wxString& GetStr() const { return m_str; }
private:
    wxString m_str;
};

wxBEGIN_EVENT_TABLE(ToolboxPanel, wxPanel)
EVT_TREE_ITEM_ACTIVATED(wxID_ANY, ToolboxPanel::OnItemActivated)
EVT_TREE_BEGIN_DRAG(wxID_ANY, ToolboxPanel::OnBeginDrag)
wxEND_EVENT_TABLE()

ToolboxPanel::ToolboxPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1))
{
    /* 1. 树占满整个面板，右边缘 0 缝隙 */
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT);
    sizer->Add(m_tree, 1, wxEXPAND | wxALL, 0);
    SetSizer(sizer);

    /* 2. 24×24 透明图标撑高节点（跨平台） */
    wxBitmap transparent(24, 24);
    transparent.SetMask(new wxMask(transparent, *wxBLACK)); // 全透明
    m_imgList = new wxImageList(24, 24, true, 10);
    m_imgList->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(24, 24))); // 0
    m_imgList->Add(transparent);                                                         // 1
    m_tree->AssignImageList(m_imgList);

    /* 3. 字体 & 缩进 */
    wxFont font(wxFontInfo(11).FaceName("Segoe UI"));
    m_tree->SetFont(font);
    m_tree->SetIndent(20);

    Rebuild();
}

void ToolboxPanel::Rebuild()
{
    m_tree->DeleteAllItems();
    wxTreeItemId root = m_tree->AddRoot("root", 0, 0);

    wxVector<wxString> mains = GetMainElements();
    wxVector<ToolCategory> cats = GetToolboxCategories();

    MY_LOG("Rebuild: main count = " + wxString::Format("%zu", mains.size()) +
        ", category count = " + wxString::Format("%zu", cats.size()));

    /* main 节点 */
    for (const auto& m : mains) {
        wxTreeItemId item = m_tree->AppendItem(root, m, 1, 1);
        m_tree->SetItemData(item, new wxStringTreeItemData(m));
    }

    /* 分类节点 */
    for (const auto& cat : cats) {
        wxTreeItemId folderId = m_tree->AppendItem(root, cat.name, 0, 0);
        m_tree->SetItemBold(folderId, true);
        for (const auto& item : cat.items) {
            wxTreeItemId leafId = m_tree->AppendItem(folderId, item, 1, 1);
            m_tree->SetItemData(leafId, new wxStringTreeItemData(item));
        }
        m_tree->Expand(folderId);
    }
}

//void ToolboxPanel::Rebuild()
//{
//    m_tree->DeleteAllItems();
//    wxTreeItemId root = m_tree->AddRoot("root", 0, 0);
//
//    /* 4. 分类数据（wxVector + push_back） */
//    struct Category {
//        wxString name;
//        wxVector<wxString> items;
//    };
//    Category cats[] = {
//        {wxT("Wiring"), {}},
//        {wxT("Gates"), {}},
//        {wxT("Memory"), {}},
//        {wxT("Input/Output"), {}},
//        {wxT("Base"), {}}
//    };
//
//    cats[0].items.push_back(wxT("Wire"));
//    cats[0].items.push_back(wxT("Splitter"));
//    cats[0].items.push_back(wxT("Pin"));
//    cats[0].items.push_back(wxT("Probe"));
//
//    cats[1].items.push_back(wxT("AND Gate"));
//    cats[1].items.push_back(wxT("OR Gate"));
//    cats[1].items.push_back(wxT("NOT Gate"));
//
//    cats[2].items.push_back(wxT("D Flip-Flop"));
//    cats[2].items.push_back(wxT("Register"));
//    cats[2].items.push_back(wxT("RAM"));
//
//    cats[3].items.push_back(wxT("Button"));
//    cats[3].items.push_back(wxT("LED"));
//    cats[3].items.push_back(wxT("7-Segment Display"));
//
//    cats[4].items.push_back(wxT("Poke Tool"));
//    cats[4].items.push_back(wxT("Edit Tool"));
//
//    /* 5. 填充树 */
//    for (const auto& cat : cats) {
//        wxTreeItemId folderId = m_tree->AppendItem(root, cat.name, 0, 0);
//        m_tree->SetItemBold(folderId, true);
//        for (const auto& item : cat.items) {
//            wxTreeItemId leafId = m_tree->AppendItem(folderId, item, 1, 1); // 1=透明图
//            m_tree->SetItemData(leafId, new wxStringTreeItemData(item));
//        }
//        m_tree->Expand(folderId);
//    }
//}

//void ToolboxPanel::OnItemActivated(wxTreeEvent& evt)
//{
//    wxString name = m_tree->GetItemText(evt.GetItem());
//    wxMessageBox("Activated: " + name);
//    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_HIGHEST + 900);
//    evt.SetString(m_tree->GetItemText(evt.GetItem()));
//    wxPostEvent(GetParent(), evt);   // 抛给 MainFrame
//}
void ToolboxPanel::OnItemActivated(wxTreeEvent& evt)
{
    wxString name = m_tree->GetItemText(evt.GetItem());
    wxCommandEvent cmdEvt(wxEVT_COMMAND_MENU_SELECTED, wxID_HIGHEST + 900);
    cmdEvt.SetString(name);
    wxPostEvent(GetParent(), cmdEvt);   // 抛给父窗口
}

void ToolboxPanel::OnBeginDrag(wxTreeEvent& evt)
{
    wxString name = m_tree->GetItemText(evt.GetItem());
    if (!name.IsEmpty()) {
        wxTextDataObject dragData(name);
        wxDropSource source(dragData, this);
        source.DoDragDrop(wxDrag_CopyOnly);
    }
}