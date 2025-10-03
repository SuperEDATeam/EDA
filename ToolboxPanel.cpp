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
#include <json/json.h>
#include "my_log.h"
#include <wx/clntdata.h>

static void MY_LOG(const wxString& s)
{
    wxFile f(wxStandardPaths::Get().GetTempDir() + "\\logsim_tools.log",
        wxFile::write_append);
    if (f.IsOpened()) {
        f.Write(wxDateTime::Now().FormatISOCombined() + "  " + s + "\n");
        f.Close();
    }
}

static const wxDataFormat myFormat("application/my-canvas-elem");



// 轻量级数据节点
class wxStringTreeItemData : public wxTreeItemData
{
public:
    explicit wxStringTreeItemData(const wxString& s) : m_str(s) {}
    const wxString& GetStr() const { return m_str; }
private:
    wxString m_str;
};



ToolboxPanel::ToolboxPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1))
{
    // ① 先创建树
    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT);

    // ② 再绑定事件（必须在 new 之后）
    m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &ToolboxPanel::OnSelectionChanged, this);
    m_tree->Bind(wxEVT_LEFT_DOWN, &ToolboxPanel::OnMouseDown, this);
    m_tree->Bind(wxEVT_TREE_BEGIN_DRAG, &ToolboxPanel::OnBeginDrag, this);

    // ③ 布局/外观（你已有的，但不要再次 new）
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_tree, 1, wxEXPAND | wxALL, 0);
    SetSizer(sizer);

    wxBitmap transparent(24, 24);
    transparent.SetMask(new wxMask(transparent, *wxBLACK));
    m_imgList = new wxImageList(24, 24, true, 10);
    m_imgList->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(24, 24)));
    m_imgList->Add(transparent);
    m_tree->AssignImageList(m_imgList);

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


void ToolboxPanel::OnItemActivated(wxTreeEvent& evt)
{
    wxString name = m_tree->GetItemText(evt.GetItem());
    MyLog("Toolbox: activated <%s>\n", name.ToUTF8().data());
    wxCommandEvent cmdEvt(wxEVT_COMMAND_MENU_SELECTED, wxID_HIGHEST + 900);
    cmdEvt.SetString(name);
    wxPostEvent(GetParent(), cmdEvt);   // 抛给父窗口
}


void ToolboxPanel::OnBeginDrag(wxTreeEvent& evt)
{
    wxString name = m_tree->GetItemText(evt.GetItem());

    MyLog("OnBeginDrag: entered, item = <%s>\n", name.ToUTF8().data());

    Json::Value root;
    root["action"] = "add";
    root["type"] = name.ToStdString();

    Json::StreamWriterBuilder w;
    std::string jsonStr = Json::writeString(w, root);   // 无换行

    MyLog("DragStart: JSON = [%s]\n", jsonStr.c_str());

    // 用自定义字节流对象
    wxCustomDataObject dragData(myFormat);
    dragData.SetData(jsonStr.size(), jsonStr.data());   // 纯字节

    wxDropSource source(dragData, this);
    MyLog("StartDrag: about to call DoDragDrop\n");
    int ret = source.DoDragDrop(wxDragCopy);
    MyLog("StartDrag: DoDragDrop returned %d\n", ret);
}

void ToolboxPanel::OnSelectionChanged(wxTreeEvent& evt)
{
    m_itemName = m_tree->GetItemText(evt.GetItem());
    MyLog("Toolbox: selection changed to <%s>\n", m_itemName.ToUTF8().data());
}

void ToolboxPanel::OnMouseDown(wxMouseEvent& evt)
{
    MyLog("OnMouseDown: entered\n");
    if (m_tree->GetSelection().IsOk()) {
        MyLog("Toolbox: single-click + mouse down\n");
        StartDrag();           // 立即启动拖放
    }
    evt.Skip();                // 让 wx 继续处理其他鼠标逻辑
}

void ToolboxPanel::StartDrag()
{
    MyLog("StartDrag: entered, name = <%s>\n", m_itemName.ToUTF8().data());



    Json::Value root;
    root["action"] = "add";
    root["type"] = m_itemName.ToStdString();

    Json::StreamWriterBuilder w;
    std::string jsonStr = Json::writeString(w, root);
    MyLog("StartDrag: JSON length = %zu bytes\n", jsonStr.size());
    wxCustomDataObject dragData(wxDataFormat("application/my-canvas-elem"));
    dragData.SetData(jsonStr.size(), jsonStr.data());
    MyLog("StartDrag: SetData %zu bytes\n", jsonStr.size());
    wxDropSource source(dragData, this);
    source.DoDragDrop(wxDragCopy);
}