#include "MainMenuBar.h"
#include "MainFrame.h"  // 为了转发调用 DoFileXXX
#include <wx/config.h>

enum
{
    idExportImage = wxID_HIGHEST + 1,
    idSaveAsNode = wxID_HIGHEST + 2,  // 保存.node文件
    idSaveAsNet = wxID_HIGHEST + 3    // 保存.net文件
};

wxBEGIN_EVENT_TABLE(MainMenuBar, wxMenuBar)

EVT_MENU(wxID_NEW, MainMenuBar::OnFileNew)
EVT_MENU(wxID_OPEN, MainMenuBar::OnFileOpen)
EVT_MENU(wxID_CLOSE, MainMenuBar::OnFileClose)
EVT_MENU(wxID_SAVE, MainMenuBar::OnFileSave)
EVT_MENU(wxID_SAVEAS, MainMenuBar::OnFileSaveAs)
EVT_MENU(idSaveAsNode, MainMenuBar::OnSaveAsNode)
EVT_MENU(idSaveAsNet, MainMenuBar::OnSaveAsNet)
EVT_MENU(idExportImage, MainMenuBar::OnExportImage)
EVT_MENU(wxID_PRINT, MainMenuBar::OnPrint)
EVT_MENU(wxID_PREFERENCES, MainMenuBar::OnPreferences)
EVT_MENU(wxID_EXIT, MainMenuBar::OnExit)
// 最近文件事件范围
EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, MainMenuBar::OnFileHistory)

EVT_MENU(wxID_UNDO, MainMenuBar::OnUndo)
EVT_MENU(wxID_CUT, MainMenuBar::OnCut)
EVT_MENU(wxID_COPY, MainMenuBar::OnCopy)
EVT_MENU(wxID_PASTE, MainMenuBar::OnPaste)
EVT_MENU(wxID_DELETE, MainMenuBar::OnDelete)
EVT_MENU(wxID_DUPLICATE, MainMenuBar::OnDuplicate)
EVT_MENU(wxID_SELECTALL, MainMenuBar::OnSelectAll)
EVT_MENU(wxID_HIGHEST + 10, MainMenuBar::OnRaiseSel)
EVT_MENU(wxID_HIGHEST + 11, MainMenuBar::OnLowerSel)
EVT_MENU(wxID_HIGHEST + 12, MainMenuBar::OnRaiseTop)
EVT_MENU(wxID_HIGHEST + 13, MainMenuBar::OnLowerBottom)
EVT_MENU(wxID_HIGHEST + 14, MainMenuBar::OnAddVertex)
EVT_MENU(wxID_HIGHEST + 15, MainMenuBar::OnRemoveVertex)

EVT_MENU(wxID_HIGHEST + 100, MainMenuBar::OnAddCircuit)
EVT_MENU(wxID_HIGHEST + 101, MainMenuBar::OnLoadLibrary)
EVT_MENU(wxID_HIGHEST + 102, MainMenuBar::OnLoadLibrary)
EVT_MENU(wxID_HIGHEST + 103, MainMenuBar::OnUnloadLibraries)
EVT_MENU(wxID_HIGHEST + 104, MainMenuBar::OnMoveCircuitUp)
EVT_MENU(wxID_HIGHEST + 105, MainMenuBar::OnMoveCircuitDown)
EVT_MENU(wxID_HIGHEST + 106, MainMenuBar::OnSetAsMainCircuit)
EVT_MENU(wxID_HIGHEST + 107, MainMenuBar::OnRemoveCircuit)
EVT_MENU(wxID_HIGHEST + 108, MainMenuBar::OnRevertAppearance)
EVT_MENU(wxID_HIGHEST + 109, MainMenuBar::OnViewToolbox)
EVT_MENU(wxID_HIGHEST + 110, MainMenuBar::OnViewSimTree)
EVT_MENU(wxID_HIGHEST + 111, MainMenuBar::OnEditLayout)
EVT_MENU(wxID_HIGHEST + 112, MainMenuBar::OnEditAppearance)
EVT_MENU(wxID_HIGHEST + 113, MainMenuBar::OnAnalyzeCircuit)
EVT_MENU(wxID_HIGHEST + 114, MainMenuBar::OnGetStats)
EVT_MENU(wxID_HIGHEST + 115, MainMenuBar::OnOptions)

EVT_MENU(wxID_HIGHEST + 200, MainMenuBar::OnSimEnable)
EVT_MENU(wxID_HIGHEST + 201, MainMenuBar::OnSimReset)
EVT_MENU(wxID_HIGHEST + 202, MainMenuBar::OnSimStep)
EVT_MENU(wxID_HIGHEST + 203, MainMenuBar::OnGoOutToState)
EVT_MENU(wxID_HIGHEST + 204, MainMenuBar::OnGoInToState)
EVT_MENU(wxID_HIGHEST + 205, MainMenuBar::OnTickOnce)
EVT_MENU(wxID_HIGHEST + 206, MainMenuBar::OnTicksEnabled)
EVT_MENU(wxID_HIGHEST + 207, MainMenuBar::OnSetTickFreq)
EVT_MENU(wxID_HIGHEST + 208, MainMenuBar::OnSetTickFreq)
EVT_MENU(wxID_HIGHEST + 209, MainMenuBar::OnSetTickFreq)
EVT_MENU(wxID_HIGHEST + 210, MainMenuBar::OnSetTickFreq)
EVT_MENU(wxID_HIGHEST + 211, MainMenuBar::OnSetTickFreq)
EVT_MENU(wxID_HIGHEST + 212, MainMenuBar::OnSetTickFreq)
EVT_MENU(wxID_HIGHEST + 213, MainMenuBar::OnLogging)

EVT_MENU(wxID_ICONIZE_FRAME, MainMenuBar::OnMinimize)
EVT_MENU(wxID_MAXIMIZE_FRAME, MainMenuBar::OnMaximize)
EVT_MENU(wxID_CLOSE_FRAME, MainMenuBar::OnCloseWnd)
EVT_MENU(wxID_HIGHEST + 300, MainMenuBar::OnCombinationalAnalysis)
EVT_MENU(wxID_HIGHEST + 301, MainMenuBar::OnWindowPreferences)

EVT_MENU(wxID_HIGHEST + 500, MainMenuBar::OnTutorial)
EVT_MENU(wxID_HIGHEST + 501, MainMenuBar::OnUserGuide)
EVT_MENU(wxID_HIGHEST + 502, MainMenuBar::OnLibraryRef)
EVT_MENU(wxID_ABOUT, MainMenuBar::OnAbout)
wxEND_EVENT_TABLE()

MainMenuBar::MainMenuBar(MainFrame* owner)
    : wxMenuBar(),
    m_owner(owner),
    m_fileHistory(9),
    m_windowMenu(nullptr)
{
    /* 1. 创建并追加六大菜单 */
    wxMenu* fileMenu = CreateFileMenu();   // 先拿到指针
    Append(fileMenu, wxT("&File"));
    Append(CreateEditMenu(), wxT("&Edit"));
    Append(CreateProjectMenu(), wxT("&Project"));
    Append(CreateSimulateMenu(), wxT("&Simulate"));
    RebuildWindowMenu();  
    Append(CreateHelpMenu(), wxT("&Help"));

    /* 2. 最近文件列表挂在 File 菜单上 */
    m_fileHistory.UseMenu(fileMenu);          // 直接用指针，不再 FindItem
    m_fileHistory.Load(*wxConfig::Get());

    /* 3. 首次重建 Window 菜单 */
    RebuildWindowMenu();
}

MainMenuBar::~MainMenuBar()
{
    SaveHistory();
}

/* ---------- 最近文件列表封装 ---------- */
void MainMenuBar::AddFileToHistory(const wxString& path)
{
    m_fileHistory.AddFileToHistory(path);
}
wxString MainMenuBar::GetHistoryFile(size_t i) const
{
    return m_fileHistory.GetHistoryFile(i);
}
size_t MainMenuBar::GetHistoryFileCount() const
{
    return m_fileHistory.GetCount();
}
void MainMenuBar::LoadHistory()
{
    wxConfig config("MyLogisim");
    m_fileHistory.Load(config);
}
void MainMenuBar::SaveHistory()
{
    wxConfig config("MyLogisim");
    m_fileHistory.Save(config);
}

/* ---------- 六大菜单创建 ---------- */
wxMenu* MainMenuBar::CreateFileMenu()
{
    wxMenu* menu = new wxMenu;
    menu->Append(wxID_NEW, "&New\tCtrl+N", "Create a new circuit");
    menu->Append(wxID_OPEN, "&Open\tCtrl+O", "Open an existing circuit");

    wxMenu* recentMenu = new wxMenu;
    menu->AppendSubMenu(recentMenu, "Open Recent");
    m_fileHistory.UseMenu(recentMenu);

    menu->AppendSeparator();
    menu->Append(wxID_CLOSE, "&Close\tCtrl+Shift+W", "Close current window");
    menu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save current circuit");
    menu->Append(wxID_SAVEAS, "Save &As...\tCtrl+Shift+S", "Save with new name");

    menu->Append(idSaveAsNode, "Save as .node", "Save nodes to .node file");
    menu->Append(idSaveAsNet, "Save as .net", "Save network to .net file");

    menu->AppendSeparator();
    menu->Append(idExportImage, "Export Image...", "Export canvas to PNG/JPEG");
    menu->Append(wxID_PRINT, "&Print...\tCtrl+P", "Print circuit");

    menu->AppendSeparator();
    menu->Append(wxID_PREFERENCES, "Preferences...", "Open settings");

    menu->AppendSeparator();
    menu->Append(wxID_EXIT, "E&xit\tCtrl+Q", "Quit application");
    return menu;
}

wxMenu* MainMenuBar::CreateEditMenu()
{
    wxMenu* m = new wxMenu;

    wxMenuItem* undoItem = new wxMenuItem(m, wxID_UNDO, "&Can't Undo\tCtrl+Z");
    undoItem->Enable(false);  // ← 关键：初始时禁用
    m->Append(undoItem);
    m->AppendSeparator();

    m->Append(wxID_CUT, "Cu&t\tCtrl+X");
    m->Append(wxID_COPY, "&Copy\tCtrl+C");
    m->Append(wxID_PASTE, "&Paste\tCtrl+V");
    m->AppendSeparator();

    //m->Append(wxID_DELETE, "&Delete\tDel");
    m->Append(wxID_DELETE, "&Delete");
    m->Append(wxID_DUPLICATE, "D&uplicate\tCtrl+D");
    m->Append(wxID_SELECTALL, "Select &All\tCtrl+A");
    m->AppendSeparator();

    /* 用 wxID_HIGHEST+10 起步，避免与系统 ID 冲突 */
    m->Append(wxID_HIGHEST + 10, "Raise Selection\tCtrl+↑");
    m->Append(wxID_HIGHEST + 11, "Lower Selection\tCtrl+↓");
    m->Append(wxID_HIGHEST + 12, "Raise to Top\tCtrl+Shift+↑");
    m->Append(wxID_HIGHEST + 13, "Lower to Bottom\tCtrl+Shift+↓");
    m->AppendSeparator();

    m->Append(wxID_HIGHEST + 14, "Add Vertex");
    m->Append(wxID_HIGHEST + 15, "Remove Vertex");

    return m;
}

wxMenu* MainMenuBar::CreateProjectMenu()
{
    wxMenu* m = new wxMenu;

    /* 1. 库管理 */
    m->Append(wxID_HIGHEST + 100, "Add Circuit...");
    wxMenu* loadSub = new wxMenu;
    loadSub->Append(wxID_HIGHEST + 101, "From Built-in Libraries");
    loadSub->Append(wxID_HIGHEST + 102, "From Jar/Directory");
    m->AppendSubMenu(loadSub, "Load Library");
    m->Append(wxID_HIGHEST + 103, "Unload Libraries...");
    m->AppendSeparator();

    /* 2. 电路排序与属性 */
    m->Append(wxID_HIGHEST + 104, "Move Circuit Up");
    m->Append(wxID_HIGHEST + 105, "Move Circuit Down");
    m->Append(wxID_HIGHEST + 106, "Set As Main Circuit");
    m->Append(wxID_HIGHEST + 107, "Remove Circuit");
    m->Append(wxID_HIGHEST + 108, "Revert to Default Appearance");
    m->AppendSeparator();

    /* 3. 视图与编辑 */
    m->Append(wxID_HIGHEST + 109, "View Toolbox");
    m->Append(wxID_HIGHEST + 110, "View Simulation Tree");
    m->Append(wxID_HIGHEST + 111, "Edit Circuit Layout");
    m->Append(wxID_HIGHEST + 112, "Edit Circuit Appearance");
    m->AppendSeparator();

    /* 4. 分析与统计 */
    m->Append(wxID_HIGHEST + 113, "Analyze Circuit");
    m->Append(wxID_HIGHEST + 114, "Get Circuit Statistics");
    m->AppendSeparator();

    /* 5. 项目级选项 */
    m->Append(wxID_HIGHEST + 115, "Options...");

    return m;
}

wxMenu* MainMenuBar::CreateSimulateMenu()
{
    wxMenu* m = new wxMenu;

    m->AppendCheckItem(wxID_HIGHEST + 200, "Simulation Enabled\tCtrl+E");
    m->Append(wxID_HIGHEST + 201, "Reset Simulation\tCtrl+R");
    m->Append(wxID_HIGHEST + 202, "Step Simulation\tCtrl+I");
    m->AppendSeparator();

    /* 状态跳转子菜单 */
    wxMenu* gotoMenu = new wxMenu;
    gotoMenu->Append(wxID_HIGHEST + 203, "Go Out To State");
    gotoMenu->Append(wxID_HIGHEST + 204, "Go In To State");
    m->AppendSubMenu(gotoMenu, "Go To State");
    m->AppendSeparator();

    /* 时钟控制 */
    m->Append(wxID_HIGHEST + 205, "Tick Once\tCtrl+T");
    m->AppendCheckItem(wxID_HIGHEST + 206, "Ticks Enabled\tCtrl+K");
    wxMenu* freqMenu = new wxMenu;
    freqMenu->Append(wxID_HIGHEST + 207, "1 Hz");
    freqMenu->Append(wxID_HIGHEST + 208, "2 Hz");
    freqMenu->Append(wxID_HIGHEST + 209, "4 Hz");
    freqMenu->Append(wxID_HIGHEST + 210, "8 Hz");
    freqMenu->Append(wxID_HIGHEST + 211, "16 Hz");
    freqMenu->Append(wxID_HIGHEST + 212, "32 Hz");
    m->AppendSubMenu(freqMenu, "Tick Frequency");
    m->AppendSeparator();

    /* 日志 */
    m->Append(wxID_HIGHEST + 213, "Logging...");

    return m;
}

void MainMenuBar::RebuildWindowMenu() //这个版本会把window放在simulate前面
{
    /* 如果已存在则先销毁旧项（重建用） */
    if (m_windowMenu)
        Remove(3);   // Window 菜单在索引 3 位置（File/Edit/Project之后）

    m_windowMenu = new wxMenu;

    /* 1. 常规窗口命令 */
    m_windowMenu->Append(wxID_ICONIZE_FRAME, "Minimize\tCtrl+M");
    m_windowMenu->Append(wxID_MAXIMIZE_FRAME, "Maximize");
    m_windowMenu->Append(wxID_CLOSE_FRAME, "Close\tCtrl+W");
    m_windowMenu->AppendSeparator();

    /* 2. 内置工具 */
    m_windowMenu->Append(wxID_HIGHEST + 300, "Combinational Analysis");
    m_windowMenu->Append(wxID_HIGHEST + 301, "Preferences");
    m_windowMenu->AppendSeparator();

    /* 3. 动态文档列表（初始 Untitled） */
    AddDocToWindowList(m_curDocTitle.IsEmpty() ? wxString("Untitled") : m_curDocTitle);
    SetCurrentDocInWindowList(m_curDocTitle.IsEmpty() ? wxString("Untitled") : m_curDocTitle);

    /* 4. 重新插入菜单栏 */
    Insert(3, m_windowMenu, wxT("&Window"));
}

//void MainMenuBar::RebuildWindowMenu()
//{
//    /* 1. 如果已存在，按名字找到并删除，避免索引错位 */
//    int oldPos = FindMenu(wxT("&Window"));
//    if (oldPos != wxNOT_FOUND)
//        Remove(oldPos);
//
//    /* 2. 重建菜单内容 */
//    m_windowMenu = new wxMenu;
//    m_windowMenu->Append(wxID_ICONIZE_FRAME, "Minimize\tCtrl+M");
//    m_windowMenu->Append(wxID_MAXIMIZE_FRAME, "Maximize");
//    m_windowMenu->Append(wxID_CLOSE_FRAME, "Close\tCtrl+W");
//    m_windowMenu->AppendSeparator();
//
//    m_windowMenu->Append(wxID_HIGHEST + 300, "Combinational Analysis");
//    m_windowMenu->Append(wxID_HIGHEST + 301, "Preferences");
//    m_windowMenu->AppendSeparator();
//
//    AddDocToWindowList(m_curDocTitle.IsEmpty() ? wxT("Untitled") : m_curDocTitle);
//    SetCurrentDocInWindowList(m_curDocTitle.IsEmpty() ? wxT("Untitled") : m_curDocTitle);
//
//    /* 3. 插在 Help 之前（Simulate 之后自然就是 Window） */
//    int helpPos = FindMenu(wxT("&Help"));
//    Insert(helpPos, m_windowMenu, wxT("&Window"));
//}

wxMenu* MainMenuBar::CreateHelpMenu()
{
    wxMenu* m = new wxMenu;
    m->Append(wxID_HIGHEST + 500, wxT("Tutorial"));
    m->Append(wxID_HIGHEST + 501, wxT("User's Guide"));
    m->Append(wxID_HIGHEST + 502, wxT("Library Reference"));
    m->AppendSeparator();
    m->Append(wxID_ABOUT, wxT("About..."));
    return m;
}

/* ---------- File 菜单事件转发 ---------- */
void MainMenuBar::OnFileNew(wxCommandEvent&)
{
    m_owner->DoFileNew();
}
void MainMenuBar::OnFileOpen(wxCommandEvent&)
{
    if (m_owner) {
        m_owner->DoFileOpen();
    }
}
void MainMenuBar::OnFileClose(wxCommandEvent&)
{
    m_owner->Close();  // 直接关窗口，后续可改
}
void MainMenuBar::OnFileSave(wxCommandEvent&)
{
    m_owner->DoFileSave();
}
void MainMenuBar::OnFileSaveAs(wxCommandEvent&)
{
    m_owner->DoFileSaveAs();
}
void MainMenuBar::OnSaveAsNode(wxCommandEvent&)
{
    if (m_owner)
        m_owner->DoFileSaveAsNode();
}

void MainMenuBar::OnSaveAsNet(wxCommandEvent&)
{
    if (m_owner)
        m_owner->DoFileSaveAsNet();
}
void MainMenuBar::OnExportImage(wxCommandEvent&)
{
    wxMessageBox("Export Image placeholder", "Info");
}
void MainMenuBar::OnPrint(wxCommandEvent&)
{
    wxMessageBox("Print placeholder", "Info");
}
void MainMenuBar::OnPreferences(wxCommandEvent&)
{
    wxMessageBox("Preferences placeholder", "Info");
}
void MainMenuBar::OnExit(wxCommandEvent&)
{
    m_owner->Close(true);
}
void MainMenuBar::OnFileHistory(wxCommandEvent& evt)
{
    const int idx = evt.GetId() - wxID_FILE1;
    wxString path = m_fileHistory.GetHistoryFile(idx);
    if (!path.IsEmpty())
    {
        m_owner->DoFileOpen(path);  // 带路径打开
        m_fileHistory.AddFileToHistory(path); // 提到最前
    }
}


void MainMenuBar::OnUndo(wxCommandEvent&) { m_owner->DoEditUndo(); }
void MainMenuBar::OnCut(wxCommandEvent&) { m_owner->DoEditCut(); }
void MainMenuBar::OnCopy(wxCommandEvent&) { m_owner->DoEditCopy(); }
void MainMenuBar::OnPaste(wxCommandEvent&) { m_owner->DoEditPaste(); }
void MainMenuBar::OnDelete(wxCommandEvent&) { m_owner->DoEditDelete(); }
void MainMenuBar::OnDuplicate(wxCommandEvent&) { m_owner->DoEditDuplicate(); }
void MainMenuBar::OnSelectAll(wxCommandEvent&) { m_owner->DoEditSelectAll(); }
void MainMenuBar::OnRaiseSel(wxCommandEvent&) { m_owner->DoEditRaiseSel(); }
void MainMenuBar::OnLowerSel(wxCommandEvent&) { m_owner->DoEditLowerSel(); }
void MainMenuBar::OnRaiseTop(wxCommandEvent&) { m_owner->DoEditRaiseTop(); }
void MainMenuBar::OnLowerBottom(wxCommandEvent&) { m_owner->DoEditLowerBottom(); }
void MainMenuBar::OnAddVertex(wxCommandEvent&) { m_owner->DoEditAddVertex(); }
void MainMenuBar::OnRemoveVertex(wxCommandEvent&) { m_owner->DoEditRemoveVertex(); }


void MainMenuBar::OnAddCircuit(wxCommandEvent&) { m_owner->DoProjectAddCircuit(); }
void MainMenuBar::OnLoadLibrary(wxCommandEvent&) { m_owner->DoProjectLoadLibrary(); }
void MainMenuBar::OnUnloadLibraries(wxCommandEvent&) { m_owner->DoProjectUnloadLibraries(); }
void MainMenuBar::OnMoveCircuitUp(wxCommandEvent&) { m_owner->DoProjectMoveCircuitUp(); }
void MainMenuBar::OnMoveCircuitDown(wxCommandEvent&) { m_owner->DoProjectMoveCircuitDown(); }
void MainMenuBar::OnSetAsMainCircuit(wxCommandEvent&) { m_owner->DoProjectSetAsMain(); }
void MainMenuBar::OnRemoveCircuit(wxCommandEvent&) { m_owner->DoProjectRemoveCircuit(); }
void MainMenuBar::OnRevertAppearance(wxCommandEvent&) { m_owner->DoProjectRevertAppearance(); }
void MainMenuBar::OnViewToolbox(wxCommandEvent&) { m_owner->DoProjectViewToolbox(); }
void MainMenuBar::OnViewSimTree(wxCommandEvent&) { m_owner->DoProjectViewSimTree(); }
void MainMenuBar::OnEditLayout(wxCommandEvent&) { m_owner->DoProjectEditLayout(); }
void MainMenuBar::OnEditAppearance(wxCommandEvent&) { m_owner->DoProjectEditAppearance(); }
void MainMenuBar::OnAnalyzeCircuit(wxCommandEvent&) { m_owner->DoProjectAnalyzeCircuit(); }
void MainMenuBar::OnGetStats(wxCommandEvent&) { m_owner->DoProjectGetStats(); }
void MainMenuBar::OnOptions(wxCommandEvent&) { m_owner->DoProjectOptions(); }


void MainMenuBar::OnSimEnable(wxCommandEvent& evt)
{
    m_owner->DoSimSetEnabled(evt.IsChecked());
}
void MainMenuBar::OnSimReset(wxCommandEvent&) { m_owner->DoSimReset(); }
void MainMenuBar::OnSimStep(wxCommandEvent&) { m_owner->DoSimStep(); }
void MainMenuBar::OnGoOutToState(wxCommandEvent&) { m_owner->DoSimGoOut(); }
void MainMenuBar::OnGoInToState(wxCommandEvent&) { m_owner->DoSimGoIn(); }
void MainMenuBar::OnTickOnce(wxCommandEvent&) { m_owner->DoSimTickOnce(); }
void MainMenuBar::OnTicksEnabled(wxCommandEvent& evt)
{
    m_owner->DoSimTicksEnabled(evt.IsChecked());
}
void MainMenuBar::OnSetTickFreq(wxCommandEvent& evt)
{
    int hz = 1 << (evt.GetId() - wxID_HIGHEST - 207);   // 1,2,4,8,16,32
    m_owner->DoSimSetTickFreq(hz);
}
void MainMenuBar::OnLogging(wxCommandEvent&) { m_owner->DoSimLogging(); }



void MainMenuBar::AddDocToWindowList(const wxString& title)
{
    /* 相同标题不再重复添加 */
    for (size_t i = 0; i < m_windowMenu->GetMenuItemCount(); ++i)
    {
        wxMenuItem* item = m_windowMenu->FindItemByPosition(i);
        if (item->GetItemLabelText() == title) return;
    }
    int id = wxID_HIGHEST + 400 + (int)m_windowMenu->GetMenuItemCount();
    m_windowMenu->Append(id, title, wxT("Switch to this document"));
    Bind(wxEVT_MENU, &MainMenuBar::OnWindowItem, this, id);
}
void MainMenuBar::SetCurrentDocInWindowList(const wxString& title)
{
    m_curDocTitle = title;
    for (size_t i = 0; i < m_windowMenu->GetMenuItemCount(); ++i)
    {
        wxMenuItem* item = m_windowMenu->FindItemByPosition(i);
        if (item->GetId() < wxID_HIGHEST + 400) continue; // 非文档项
        wxString label = item->GetItemLabelText();
        /* 去掉旧圆点 */
        if (label.StartsWith(wxT("• "))) label = label.Mid(2);
        /* 给当前项加圆点 */
        if (label == title) label = wxT("• ") + label;
        item->SetItemLabel(label);
    }
}
void MainMenuBar::OnMinimize(wxCommandEvent&) { m_owner->Iconize(true); }
void MainMenuBar::OnMaximize(wxCommandEvent&) { m_owner->Maximize(); }
void MainMenuBar::OnCloseWnd(wxCommandEvent&) { m_owner->Close(); }
void MainMenuBar::OnCombinationalAnalysis(wxCommandEvent&)
{
    m_owner->DoWindowCombinationalAnalysis();
}
void MainMenuBar::OnWindowPreferences(wxCommandEvent&)
{
    m_owner->DoWindowPreferences();
}
void MainMenuBar::OnWindowItem(wxCommandEvent& evt)
{
    wxString title = m_windowMenu->FindItem(evt.GetId())->GetItemLabelText();
    if (title.StartsWith(wxT("• "))) title = title.Mid(2); // 去掉圆点
    m_owner->DoWindowSwitchToDoc(title);
}

void MainMenuBar::OnTutorial(wxCommandEvent&)
{
    m_owner->DoHelpTutorial();
}
void MainMenuBar::OnUserGuide(wxCommandEvent&)
{
    m_owner->DoHelpUserGuide();
}
void MainMenuBar::OnLibraryRef(wxCommandEvent&)
{
    m_owner->DoHelpLibraryRef();
}
void MainMenuBar::OnAbout(wxCommandEvent&)
{
    m_owner->DoHelpAbout();
}