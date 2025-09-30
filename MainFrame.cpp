#include "MainFrame.h"
#include "MainMenuBar.h"
#include "CanvasPanel.h"
#include "PropertyPanel.h"
#include <wx/msgdlg.h>
#include "ToolboxPanel.h"   // 你的侧边栏
#include <wx/aui/aui.h>

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
EVT_MENU(wxID_HIGHEST + 900, MainFrame::OnToolboxElement)
wxEND_EVENT_TABLE()


MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "MyLogisim")
{
    /* 菜单 & 状态栏 */
    SetMenuBar(new MainMenuBar(this));
    CreateStatusBar(1);
    SetStatusText("Ready");
    SetTitle("Untitled");
    static_cast<MainMenuBar*>(GetMenuBar())->SetCurrentDocInWindowList("Untitled");

    /* 把窗口交给 AUI 管理（必须最先） */
    m_auiMgr.SetManagedWindow(this);

    /* 创建中央画布（先空白占位） */
    CanvasPanel* canvas = new CanvasPanel(this);
    canvas->SetBackgroundColour(*wxWHITE);

    /* 创建侧边栏 + 属性表（上下叠放） */
    wxPanel* sidePanel = new wxPanel(this);  // 外壳
    wxBoxSizer* sideSizer = new wxBoxSizer(wxVERTICAL);

    ToolboxPanel* toolbox = new ToolboxPanel(sidePanel);  // 父窗口是 sidePanel
    sideSizer->Add(toolbox, 1, wxEXPAND);    // 上：工具树（可拉伸）

    m_propPanel = new PropertyPanel(sidePanel);  // 父窗口是 sidePanel
    sideSizer->Add(m_propPanel, 0, wxEXPAND);    // 下：属性表（先固定高）

    sidePanel->SetSizer(sideSizer);

    /* 把整个侧边区作为一个 AUI Pane 停靠 */
    m_auiMgr.AddPane(sidePanel, wxAuiPaneInfo()
        .Name("side")               // 统一名字
        .Caption("Toolbox & Properties")
        .Left()
        .Layer(1)
        .Position(1)
        .CloseButton(false)
        .BestSize(280, 700)        // 总高度留给两部分
        .MinSize(200, 400)
        .FloatingSize(280, 700)
        .Gripper(true)
        .PaneBorder(false));

    /* 画布保持原样 */
    m_auiMgr.AddPane(canvas, wxAuiPaneInfo()
        .Name("canvas")
        .CenterPane()
        .CloseButton(false)
        .MinSize(400, 300));

    /* 一次性提交 */
    m_auiMgr.Update();
}

MainFrame::~MainFrame()
{
    m_auiMgr.UnInit();   // 必须手动反初始化
}

void MainFrame::DoFileNew() { wxMessageBox("DoFileNew"); }
void MainFrame::DoFileSave() { wxMessageBox("DoFileSave"); }
void MainFrame::DoFileSaveAs() { wxMessageBox("DoFileSaveAs"); }
void MainFrame::DoFileOpen(const wxString& path)
{
    wxString msg = path.IsEmpty()
        ? wxT("DoFileOpen dialog")
        : wxString::Format(wxT("DoFileOpen: %s"), path);
    wxMessageBox(msg, wxT("Info"), wxOK | wxICON_INFORMATION, this);
}
void MainFrame::OnAbout(wxCommandEvent&)
{
    wxMessageBox(wxString::Format(wxT("MyLogisim\n%s"), wxVERSION_STRING),
        wxT("About"), wxOK | wxICON_INFORMATION, this);
}

void MainFrame::DoEditUndo() { wxMessageBox("Edit->Undo"); }
void MainFrame::DoEditCut() { wxMessageBox("Edit->Cut"); }
void MainFrame::DoEditCopy() { wxMessageBox("Edit->Copy"); }
void MainFrame::DoEditPaste() { wxMessageBox("Edit->Paste"); }
void MainFrame::DoEditDelete() { wxMessageBox("Edit->Delete"); }
void MainFrame::DoEditDuplicate() { wxMessageBox("Edit->Duplicate"); }
void MainFrame::DoEditSelectAll() { wxMessageBox("Edit->SelectAll"); }
void MainFrame::DoEditRaiseSel() { wxMessageBox("Edit->Raise Selection"); }
void MainFrame::DoEditLowerSel() { wxMessageBox("Edit->Lower Selection"); }
void MainFrame::DoEditRaiseTop() { wxMessageBox("Edit->Raise to Top"); }
void MainFrame::DoEditLowerBottom() { wxMessageBox("Edit->Lower to Bottom"); }
void MainFrame::DoEditAddVertex() { wxMessageBox("Edit->Add Vertex"); }
void MainFrame::DoEditRemoveVertex() { wxMessageBox("Edit->Remove Vertex"); }

void MainFrame::DoProjectAddCircuit() { wxMessageBox("Project->Add Circuit"); }
void MainFrame::DoProjectLoadLibrary() { wxMessageBox("Project->Load Library"); }
void MainFrame::DoProjectUnloadLibraries() { wxMessageBox("Project->Unload Libraries"); }
void MainFrame::DoProjectMoveCircuitUp() { wxMessageBox("Project->Move Circuit Up"); }
void MainFrame::DoProjectMoveCircuitDown() { wxMessageBox("Project->Move Circuit Down"); }
void MainFrame::DoProjectSetAsMain() { wxMessageBox("Project->Set As Main Circuit"); }
void MainFrame::DoProjectRemoveCircuit() { wxMessageBox("Project->Remove Circuit"); }
void MainFrame::DoProjectRevertAppearance() { wxMessageBox("Project->Revert Appearance"); }
void MainFrame::DoProjectViewToolbox() { wxMessageBox("Project->View Toolbox"); }
void MainFrame::DoProjectViewSimTree() { wxMessageBox("Project->View Simulation Tree"); }
void MainFrame::DoProjectEditLayout() { wxMessageBox("Project->Edit Circuit Layout"); }
void MainFrame::DoProjectEditAppearance() { wxMessageBox("Project->Edit Circuit Appearance"); }
void MainFrame::DoProjectAnalyzeCircuit() { wxMessageBox("Project->Analyze Circuit"); }
void MainFrame::DoProjectGetStats() { wxMessageBox("Project->Get Circuit Statistics"); }
void MainFrame::DoProjectOptions() { wxMessageBox("Project->Options"); }

void MainFrame::DoSimSetEnabled(bool on)
{
    wxMessageBox(wxString::Format("Simulation %s", on ? "enabled" : "disabled"));
}
void MainFrame::DoSimReset() { wxMessageBox("Simulation reset"); }
void MainFrame::DoSimStep() { wxMessageBox("Simulation step"); }
void MainFrame::DoSimGoOut() { wxMessageBox("Go Out To State"); }
void MainFrame::DoSimGoIn() { wxMessageBox("Go In To State"); }
void MainFrame::DoSimTickOnce() { wxMessageBox("Tick Once"); }
void MainFrame::DoSimTicksEnabled(bool on)
{
    wxMessageBox(wxString::Format("Ticks %s", on ? "enabled" : "disabled"));
}
void MainFrame::DoSimSetTickFreq(int hz)
{
    wxMessageBox(wxString::Format("Tick frequency set to %d Hz", hz));
}
void MainFrame::DoSimLogging() { wxMessageBox("Logging dialog"); }

void MainFrame::DoWindowCombinationalAnalysis()
{
    wxMessageBox("Window->Combinational Analysis");
}
void MainFrame::DoWindowPreferences()
{
    wxMessageBox("Window->Preferences");
}
void MainFrame::DoWindowSwitchToDoc(const wxString& title)
{
    wxMessageBox("Switch to document: " + title);
    // 真正逻辑：把主框架切换到对应子窗口或视图
}

void MainFrame::DoHelpTutorial()
{
    wxMessageBox("Help->Tutorial");
}
void MainFrame::DoHelpUserGuide()
{
    wxMessageBox("Help->User's Guide");
}
void MainFrame::DoHelpLibraryRef()
{
    wxMessageBox("Help->Library Reference");
}
void MainFrame::DoHelpAbout()
{
    wxMessageBox(wxString::Format("MyLogisim\n%s", wxVERSION_STRING),
        wxT("About"), wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnToolboxElement(wxCommandEvent& evt)
{
    m_propPanel->ShowElement(evt.GetString());
}