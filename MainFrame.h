#pragma once
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include "PropertyPanel.h"
#include "CanvasPanel.h"
#include "3rd/json/json.h"
#include <wx/file.h>  // ����wxFile��Ķ���
#include <wx/xml/xml.h>
#include <wx/mstream.h>
#include "ToolBars.h"

class ToolBars;   // ǰ������
class CanvasPanel; // ǰ������
class HandyToolKit;

class MainFrame : public wxFrame
{



private:
    // �����ﶨ�� m_currentFilePath
    wxString m_currentFilePath;  // ��¼��ǰ�ļ��ı���·����Ϊ�ձ�ʾδ���棩
    bool m_isModified;           // ����ļ��Ƿ��޸ģ���ϱ����߼���

    // ����˽�г�Ա����...
private:
    // ˽�и�����������...
    bool SaveToFile(const wxString& filePath);
    wxString GenerateFileContent();
private:
    void AddLibraryNode(wxXmlNode* parent, const wxString& name, const wxString& desc);
    void AddWireNode(wxXmlNode* parent, const wxString& from, const wxString& to);
    bool SaveAsNodeFile(const wxString& filePath);
    bool SaveAsNetFile(const wxString& filePath);
    // �������ر��¼�������������
    void OnClose(wxCloseEvent& event);
    // ...
    void OnToolSelected(wxCommandEvent& evt);  // ��������ѡ���¼�

public:
    MainFrame();
    ~MainFrame();

    void OnUndoStackChanged();

    /* File �˵�ҵ��ӿ� */
    void DoFileNew();
    void DoFileOpen(const wxString& path = {});
    void DoFileSave();
    void DoFileSaveAs();
    void DoFileSaveAsNode();  // ����Ϊ.node�ļ�
    void DoFileSaveAsNet();   // ����Ϊ.net�ļ�
    void OnQuit(wxCommandEvent&) { Close(true); }
    void OnAbout(wxCommandEvent&);

    /* Edit �˵�ҵ��ӿ� */
    void DoEditUndo();
    void DoEditCut();
    void DoEditCopy();
    void DoEditPaste();
    void DoEditDelete();
    void DoEditDuplicate();
    void DoEditSelectAll();
    void DoEditRaiseSel();
    void DoEditLowerSel();
    void DoEditRaiseTop();
    void DoEditLowerBottom();
    void DoEditAddVertex();
    void DoEditRemoveVertex();

    /* Project �˵�ҵ��ӿ� */
    void DoProjectAddCircuit();
    void DoProjectLoadLibrary();
    void DoProjectUnloadLibraries();
    void DoProjectMoveCircuitUp();
    void DoProjectMoveCircuitDown();
    void DoProjectSetAsMain();
    void DoProjectRemoveCircuit();
    void DoProjectRevertAppearance();
    void DoProjectViewToolbox();
    void DoProjectViewSimTree();
    void DoProjectEditLayout();
    void DoProjectEditAppearance();
    void DoProjectAnalyzeCircuit();
    void DoProjectGetStats();
    void DoProjectOptions();

    /* Simulate �˵�ҵ��ӿ� */
    void DoSimSetEnabled(bool on);
    void DoSimReset();
    void DoSimStep();
    void DoSimGoOut();
    void DoSimGoIn();
    void DoSimTickOnce();
    void DoSimTicksEnabled(bool on);
    void DoSimSetTickFreq(int hz);
    void DoSimLogging();

    /* Window �˵�ҵ��ӿ� */
    void DoWindowCombinationalAnalysis();
    void DoWindowPreferences();
    void DoWindowSwitchToDoc(const wxString& title);

    /* Help �˵�ҵ��ӿ� */
    void DoHelpTutorial();
    void DoHelpUserGuide();
    void DoHelpLibraryRef();
    void DoHelpAbout();

    // ��������ʽ
    ToolBars* m_toolBars;
    void AddToolBarsToAuiManager();

private:
    wxAuiManager m_auiMgr;
    PropertyPanel* m_propPanel = nullptr;
    CanvasPanel* m_canvas;

    void UpdateCursor();        // ���� m_pendingTool ����ʮ��/������

    void OnToolboxElement(wxCommandEvent& evt);
    wxDECLARE_EVENT_TABLE();
};