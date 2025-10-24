#pragma once
#include <wx/wx.h>
#include <wx/filehistory.h>

class MainFrame;  // ǰ������

/*
 * ��ʱֻʵ�� File �˵�������˵�����
 * �¼�ȫ��ת���� MainFrame �� DoFileXXX() �ӿ�
 */
class MainMenuBar : public wxMenuBar
{
public:
    explicit MainMenuBar(MainFrame* owner);
    ~MainMenuBar();

    /* ����ļ��б��װ */
    void    AddFileToHistory(const wxString& fullPath);
    wxString GetHistoryFile(size_t i) const;
    size_t  GetHistoryFileCount() const;
    void    LoadHistory();
    void    SaveHistory();

    /* �� MainFrame ���õĽӿ� */
    void RebuildWindowMenu();
    void AddDocToWindowList(const wxString& title);
    void SetCurrentDocInWindowList(const wxString& title);

private:
    MainFrame* m_owner;
    wxFileHistory m_fileHistory;

    wxMenu* m_windowMenu;        // ����ָ�룬�����ؽ�
    wxString m_curDocTitle;      // ��ǰ�ĵ����⣨����·����

    /* ����˵��������� */
    wxMenu* CreateFileMenu();
    wxMenu* CreateEditMenu();
    wxMenu* CreateProjectMenu();
    wxMenu* CreateSimulateMenu();
    wxMenu* CreateWindowMenu();
    wxMenu* CreateHelpMenu();

    /* File �˵��¼��ص� */
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

    /* Edit �˵��¼��ص� */
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

    /* Project �˵��¼��ص� */
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

    /* Simulate �˵��¼��ص� */
    void OnSimEnable(wxCommandEvent&);
    void OnSimReset(wxCommandEvent&);
    void OnSimStep(wxCommandEvent&);
    void OnGoOutToState(wxCommandEvent&);
    void OnGoInToState(wxCommandEvent&);
    void OnTickOnce(wxCommandEvent&);
    void OnTicksEnabled(wxCommandEvent&);
    void OnSetTickFreq(wxCommandEvent&);
    void OnLogging(wxCommandEvent&);

    /* Window �˵��¼��ص� */
    void OnMinimize(wxCommandEvent&);
    void OnMaximize(wxCommandEvent&);
    void OnCloseWnd(wxCommandEvent&);
    void OnCombinationalAnalysis(wxCommandEvent&);
    void OnWindowPreferences(wxCommandEvent&);
    void OnWindowItem(wxCommandEvent&);          // ��̬�ĵ��б�ͳһ���

    /* Window �˵�ҵ��ӿ� */
    void DoWindowCombinationalAnalysis();
    void DoWindowPreferences();
    void DoWindowSwitchToDoc(const wxString& title);

    /* Help �˵��¼��ص� */
    void OnTutorial(wxCommandEvent&);
    void OnUserGuide(wxCommandEvent&);
    void OnLibraryRef(wxCommandEvent&);
    void OnAbout(wxCommandEvent&);
    DECLARE_EVENT_TABLE()
};
