#pragma once
#include <wx/wx.h>
#include <wx/filehistory.h>

class MainFrame;  // 前向声明

/*
 * 暂时只实现 File 菜单，其余菜单留空
 * 事件全部转发到 MainFrame 的 DoFileXXX() 接口
 */
class MainMenuBar : public wxMenuBar
{
public:
    explicit MainMenuBar(MainFrame* owner);
    ~MainMenuBar();

    /* 最近文件列表封装 */
    void    AddFileToHistory(const wxString& fullPath);
    wxString GetHistoryFile(size_t i) const;
    size_t  GetHistoryFileCount() const;
    void    LoadHistory();
    void    SaveHistory();

    /* 让 MainFrame 调用的接口 */
    void RebuildWindowMenu();
    void AddDocToWindowList(const wxString& title);
    void SetCurrentDocInWindowList(const wxString& title);

private:
    MainFrame* m_owner;
    wxFileHistory m_fileHistory;

    wxMenu* m_windowMenu;        // 保存指针，方便重建
    wxString m_curDocTitle;      // 当前文档标题（不含路径）

    /* 六大菜单创建函数 */
    wxMenu* CreateFileMenu();
    wxMenu* CreateEditMenu();
    wxMenu* CreateProjectMenu();
    wxMenu* CreateSimulateMenu();
    wxMenu* CreateWindowMenu();
    wxMenu* CreateHelpMenu();

    /* File 菜单事件回调 */
    void OnFileNew(wxCommandEvent&);
    void OnFileOpen(wxCommandEvent&);
    void OnFileClose(wxCommandEvent&);
    void OnFileSave(wxCommandEvent&);
    void OnFileSaveAs(wxCommandEvent&);
    void OnFileHistory(wxCommandEvent&);
    void OnExportImage(wxCommandEvent&);
    void OnPrint(wxCommandEvent&);
    void OnPreferences(wxCommandEvent&);
    void OnExit(wxCommandEvent&);

    /* Edit 菜单事件回调 */
    void OnUndo(wxCommandEvent&);
    void OnCut(wxCommandEvent&);
    void OnCopy(wxCommandEvent&);
    void OnPaste(wxCommandEvent&);
    void OnDelete(wxCommandEvent&);
    void OnDuplicate(wxCommandEvent&);
    void OnSelectAll(wxCommandEvent&);
    void OnRaiseSel(wxCommandEvent&);
    void OnLowerSel(wxCommandEvent&);
    void OnRaiseTop(wxCommandEvent&);
    void OnLowerBottom(wxCommandEvent&);
    void OnAddVertex(wxCommandEvent&);
    void OnRemoveVertex(wxCommandEvent&);

    /* Project 菜单事件回调 */
    void OnAddCircuit(wxCommandEvent&);
    void OnLoadLibrary(wxCommandEvent&);
    void OnUnloadLibraries(wxCommandEvent&);
    void OnMoveCircuitUp(wxCommandEvent&);
    void OnMoveCircuitDown(wxCommandEvent&);
    void OnSetAsMainCircuit(wxCommandEvent&);
    void OnRemoveCircuit(wxCommandEvent&);
    void OnRevertAppearance(wxCommandEvent&);
    void OnViewToolbox(wxCommandEvent&);
    void OnViewSimTree(wxCommandEvent&);
    void OnEditLayout(wxCommandEvent&);
    void OnEditAppearance(wxCommandEvent&);
    void OnAnalyzeCircuit(wxCommandEvent&);
    void OnGetStats(wxCommandEvent&);
    void OnOptions(wxCommandEvent&);

    /* Simulate 菜单事件回调 */
    void OnSimEnable(wxCommandEvent&);
    void OnSimReset(wxCommandEvent&);
    void OnSimStep(wxCommandEvent&);
    void OnGoOutToState(wxCommandEvent&);
    void OnGoInToState(wxCommandEvent&);
    void OnTickOnce(wxCommandEvent&);
    void OnTicksEnabled(wxCommandEvent&);
    void OnSetTickFreq(wxCommandEvent&);
    void OnLogging(wxCommandEvent&);

    /* Window 菜单事件回调 */
    void OnMinimize(wxCommandEvent&);
    void OnMaximize(wxCommandEvent&);
    void OnCloseWnd(wxCommandEvent&);
    void OnCombinationalAnalysis(wxCommandEvent&);
    void OnWindowPreferences(wxCommandEvent&);
    void OnWindowItem(wxCommandEvent&);          // 动态文档列表统一入口

    /* Window 菜单业务接口 */
    void DoWindowCombinationalAnalysis();
    void DoWindowPreferences();
    void DoWindowSwitchToDoc(const wxString& title);

    /* Help 菜单事件回调 */
    void OnTutorial(wxCommandEvent&);
    void OnUserGuide(wxCommandEvent&);
    void OnLibraryRef(wxCommandEvent&);
    void OnAbout(wxCommandEvent&);
    DECLARE_EVENT_TABLE()
};
