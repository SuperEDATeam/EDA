#pragma once
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include "PropertyPanel.h"
#include "CanvasPanel.h"
#include "3rd/json/json.h"
#include <wx/file.h>  // 包含wxFile类的定义
#include <wx/xml/xml.h>
#include <wx/mstream.h>
#include "ToolBars.h"

class ToolBars;   // 前向声明
class CanvasPanel; // 前向声明
class HandyToolKit;

class MainFrame : public wxFrame
{



private:
    // 在这里定义 m_currentFilePath
    wxString m_currentFilePath;  // 记录当前文件的保存路径（为空表示未保存）
    bool m_isModified;           // 标记文件是否被修改（配合保存逻辑）

    // 其他私有成员变量...
private:
    // 私有辅助方法声明...
    bool SaveToFile(const wxString& filePath);
    wxString GenerateFileContent();
private:
    void AddLibraryNode(wxXmlNode* parent, const wxString& name, const wxString& desc);
    void AddWireNode(wxXmlNode* parent, const wxString& from, const wxString& to);
    bool SaveAsNodeFile(const wxString& filePath);
    bool SaveAsNetFile(const wxString& filePath);
    // 新增：关闭事件处理函数声明
    void OnClose(wxCloseEvent& event);
    // ...

public:
    MainFrame();
    ~MainFrame();

    void OnUndoStackChanged();

    /* File 菜单业务接口 */
    void DoFileNew();
    void DoFileOpen(const wxString& path = {});
    void DoFileSave();
    void DoFileSaveAs();
    void DoFileSaveAsNode();  // 保存为.node文件
    void DoFileSaveAsNet();   // 保存为.net文件
    void OnQuit(wxCommandEvent&) { Close(true); }
    void OnAbout(wxCommandEvent&);

    /* Edit 菜单业务接口 */
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

    /* Project 菜单业务接口 */
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

    /* Simulate 菜单业务接口 */
    void DoSimSetEnabled(bool on);
    void DoSimReset();
    void DoSimStep();
    void DoSimGoOut();
    void DoSimGoIn();
    void DoSimTickOnce();
    void DoSimTicksEnabled(bool on);
    void DoSimSetTickFreq(int hz);
    void DoSimLogging();

    /* Window 菜单业务接口 */
    void DoWindowCombinationalAnalysis();
    void DoWindowPreferences();
    void DoWindowSwitchToDoc(const wxString& title);

    /* Help 菜单业务接口 */
    void DoHelpTutorial();
    void DoHelpUserGuide();
    void DoHelpLibraryRef();
    void DoHelpAbout();

    // 工具栏样式
    ToolBars* m_toolBars;
    void AddToolBarsToAuiManager();

private:
    wxAuiManager m_auiMgr;
    PropertyPanel* m_propPanel = nullptr;
    CanvasPanel* m_canvas;

    void UpdateCursor();        // 根据 m_pendingTool 设置十字/常规光标

    void OnToolboxElement(wxCommandEvent& evt);
    wxDECLARE_EVENT_TABLE();
};