#include "MainFrame.h"
#include "MainMenuBar.h"
#include "CanvasPanel.h"
#include "PropertyPanel.h"
#include <wx/msgdlg.h>
#include "ToolboxPanel.h"   // ��Ĳ����
#include <wx/aui/aui.h>
#include "CanvasModel.h"
#include "my_log.h"
#include <wx/filename.h> 
#include <wx/sstream.h>
#include "ToolBars.h"

extern std::vector<CanvasElement> g_elements;

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
EVT_MENU(wxID_HIGHEST + 900, MainFrame::OnToolboxElement)
wxEND_EVENT_TABLE()


MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "MyLogisim")
{

    wxString jsonPath = wxFileName(wxGetCwd(), "canvas_elements.json").GetFullPath();
    MyLog("MainFrame: JSON full path = [%s]\n", jsonPath.ToUTF8().data());
    g_elements = LoadCanvasElements(jsonPath);

    ///* �˵� & ״̬�� */
    SetMenuBar(new MainMenuBar(this));
    CreateStatusBar(1);
    SetStatusText("Ready");
    SetTitle("Untitled");
    static_cast<MainMenuBar*>(GetMenuBar())->SetCurrentDocInWindowList("Untitled");



    ///* �Ѵ��ڽ��� AUI �����������ȣ� */
    m_auiMgr.SetManagedWindow(this);

    //������
    m_toolBars = new ToolBars(this);
    AddToolBarsToAuiManager();

    ///* �������뻭�����ȿհ�ռλ�� */
    m_canvas = new CanvasPanel(this);
    m_canvas->SetBackgroundColour(*wxWHITE);

    ///* ��������� + ���Ա����µ��ţ� */
    wxPanel* sidePanel = new wxPanel(this);  // ���
    wxBoxSizer* sideSizer = new wxBoxSizer(wxVERTICAL);

    ToolboxPanel* toolbox = new ToolboxPanel(sidePanel);  // �������� sidePanel
    sideSizer->Add(toolbox, 1, wxEXPAND);    // �ϣ��������������죩

    m_propPanel = new PropertyPanel(sidePanel);  // �������� sidePanel
    sideSizer->Add(m_propPanel, 0, wxEXPAND);    // �£����Ա��ȹ̶��ߣ�

    sidePanel->SetSizer(sideSizer);

    ///* �������������Ϊһ�� AUI Pane ͣ�� */
    m_auiMgr.AddPane(sidePanel, wxAuiPaneInfo()
        .Name("side")               // ͳһ����
        .Caption("Toolbox & Properties")
        .Left()
        .Layer(1)
        .Position(1)
        .CloseButton(false)
        .BestSize(280, 700)        // �ܸ߶���������/��
        .MinSize(200, 400)
        .FloatingSize(280, 700)
        .Gripper(true)
        .PaneBorder(false));

    ///* ��������ԭ�� */
    m_auiMgr.AddPane(m_canvas, wxAuiPaneInfo()
        .Name("canvas")
        .CenterPane()
        .CloseButton(false)
        .MinSize(400, 300));

    ///* һ�����ύ */
    m_auiMgr.Update();

    m_pendingTool.Clear();
    UpdateCursor();
}

MainFrame::~MainFrame()
{
    m_auiMgr.UnInit();   // �����ֶ�����ʼ��
}


//void MainFrame::OnToolboxElement(wxCommandEvent& evt)
//{
//    MyLog("MainFrame: received <%s>\n", evt.GetString().ToUTF8().data());
//
//    wxString name = evt.GetString();
//    auto it = std::find_if(g_elements.begin(), g_elements.end(),
//        [&](const CanvasElement& e) { return e.GetName() == name; });
//    if (it == g_elements.end()) return;
//
//    CanvasElement clone = *it;
//    clone.SetPos(wxPoint(100, 100));   // �ȷ����룬�������������
//    m_canvas->AddElement(clone);        
//}

void MainFrame::OnToolboxElement(wxCommandEvent& evt)
{
    m_pendingTool = evt.GetString();
    UpdateCursor();
    SetStatusText(wxString::Format("Select position to place <%s>", m_pendingTool));
}

const wxString& MainFrame::GetPendingTool() const { return m_pendingTool; }
void MainFrame::ClearPendingTool()
{
    m_pendingTool.Clear();
    UpdateCursor();
    SetStatusText("Ready");
}

//Ŀǰֻ�޸���Mainframe�������˽�б������������Լ�jsonͷ�ļ�
// �޸�.h�ļ�GenerateFileContent()������
//TODO 9.24
//ֻ�Ǵ�һ���´��ڣ���������д������κθı�
void MainFrame::DoFileNew() {
    // ֱ�Ӵ���һ���µĿհ�MainFrame����
    MainFrame* newFrame = new MainFrame();  // ����MainFrame���캯�����ʼ���հ�״̬

    // ��ʾ�´��ڣ�������ʾ��
    newFrame->Centre(wxBOTH);
    newFrame->Show(true);

    // ����ѡ�������Ҫ��¼���д򿪵Ĵ��ڣ�����ӵ������б���
    // m_allFrames.push_back(newFrame);  // ��Ҫ��MainFrame��������m_allFrames
}









//�����ļ���ʵ�֣����������ĸ�����
void MainFrame::DoFileSave() {
    // 1. �����ǰ�ĵ�û��·����δ��������������"���Ϊ"
    if (m_currentFilePath.IsEmpty()) {
        // ����DoFileSaveAs()�����״α��棨��ʵ�ָ÷�����
        DoFileSaveAs();
        return;
    }

    // 2. ���Խ���ǰ�ĵ�����д���ļ�
    bool saveSuccess = SaveToFile(m_currentFilePath);

    // 3. ���ݱ���������״̬
    if (saveSuccess) {
        m_isModified = false;  // ����ɹ������Ϊδ�޸�
        //UpdateTitle();         // ���´��ڱ��⣨�Ƴ�"*"���޸ı�ǣ�
        SetStatusText(wxString::Format("�ѱ���: %s", m_currentFilePath));
    }
    else {
        wxMessageBox(
            wxString::Format("����ʧ��: %s", m_currentFilePath),
            "����",
            wxOK | wxICON_ERROR,
            this
        );
    }
}

// ��������������ǰ�ĵ�����д��ָ��·�����޸�ΪXML��ʽ��
bool MainFrame::SaveToFile(const wxString& filePath) {
    // 1. ����XML����
    wxString xmlContent = GenerateFileContent();
    if (xmlContent.IsEmpty()) {
        return false;
    }

    // 2. ����ļ��Ƿ���ڣ��������򴴽�
    if (!wxFile::Exists(filePath)) {
        wxFile tempFile;
        if (!tempFile.Open(filePath, wxFile::write)) {
            return false;
        }
        tempFile.Close();
    }

    // 3. ���ļ���д��XML����
    wxFile file;
    if (!file.Open(filePath, wxFile::write)) {
        return false;
    }

    // 4. д�벢�ر��ļ�
    size_t bytesWritten = file.Write(xmlContent);
    file.Close();

    // 5. ��֤д����
    return bytesWritten == xmlContent.Length();
}




wxString MainFrame::GenerateFileContent()
{
    // 1. �����ĵ���**��**�ֹ�ƴ XML ����
    wxXmlDocument doc;

    // 2. ֱ�ӹҸ��ڵ� <project>
    wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "project");
    root->AddAttribute("source", "2.7.1");
    root->AddAttribute("version", "1.0");
    doc.SetRoot(root);

    // 3. ע��
    root->AddChild(new wxXmlNode(wxXML_COMMENT_NODE,
        "This file is intended to be loaded by Logisim "
        "(http://www.cburch.com/logisim/)"));

    // 4. Ԫ����
    auto AddLib = [&](const wxString& desc, const wxString& name)
        {
            wxXmlNode* lib = new wxXmlNode(wxXML_ELEMENT_NODE, "lib");
            lib->AddAttribute("desc", desc);
            lib->AddAttribute("name", name);
            root->AddChild(lib);
        };
    AddLib("#Wiring", "0");
    AddLib("#Gates", "1");

    // 5. ����·
    wxXmlNode* circuit = new wxXmlNode(wxXML_ELEMENT_NODE, "circuit");
    circuit->AddAttribute("name", "main");
    root->AddChild(circuit);

    // 6. ʾ������
    wxXmlNode* wire = new wxXmlNode(wxXML_ELEMENT_NODE, "wire");
    wire->AddAttribute("from", "(370,350)");
    wire->AddAttribute("to", "(430,350)");
    circuit->AddChild(wire);

    // 7. �ڴ���ַ�����Unicode ��ȫ��
    wxStringOutputStream strStream;
    doc.Save(strStream, wxXML_DOCUMENT_TYPE_NODE); // �Զ��� XML ����
    return strStream.GetString();
}


// �������������Ԫ����ڵ�
void MainFrame::AddLibraryNode(wxXmlNode* parent, const wxString& name, const wxString& desc) {
    wxXmlNode* lib = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("lib"));
    lib->AddAttribute(wxT("name"), name);
    lib->AddAttribute(wxT("desc"), desc);
    parent->AddChild(lib);
}

// ������������ӵ��߽ڵ�
void MainFrame::AddWireNode(wxXmlNode* parent, const wxString& from, const wxString& to) {
    wxXmlNode* wire = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("wire"));
    wire->AddAttribute(wxT("from"), from);
    wire->AddAttribute(wxT("to"), to);
    parent->AddChild(wire);
}
// ������ʵ��"���Ϊ"�����������״α��棩
void MainFrame::DoFileSaveAs() {
    // �����ļ�ѡ��Ի���
    wxFileDialog saveDialog(
        this,
        "���Ϊ",
        "",
        "Untitled.circ",  // Ĭ���ļ���
        "��·�ļ� (*.circ)|*.circ|�����ļ� (*.*)|*.*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT  // ��ʾ���������ļ�
    );

    // �û�ȡ���򷵻�
    if (saveDialog.ShowModal() != wxID_OK) {
        return;
    }

    // ��ȡ�û�ѡ���·��
    wxString newPath = saveDialog.GetPath();
    m_currentFilePath = newPath;

    // ִ�б���
    DoFileSave();

    // ����ѡ����ӵ�����ļ���ʷ
    static_cast<MainMenuBar*>(GetMenuBar())->AddFileToHistory(newPath);
}



void MainFrame::DoFileOpen(const wxString& path)
{
    wxString msg = path.IsEmpty()
        ? wxString("DoFileOpen dialog")     // ǿ�� wxString
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
    // �����߼�����������л�����Ӧ�Ӵ��ڻ���ͼ
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

void MainFrame::UpdateCursor()
{
    if (m_pendingTool.IsEmpty())
        SetCursor(wxCursor(wxCURSOR_ARROW));
    else
        SetCursor(wxCursor(wxCURSOR_CROSS));
}

// �� MainFrame ��������������
void MainFrame::AddToolBarsToAuiManager() {
    if (!m_toolBars) return;

    wxToolBar* toolBar1 = m_toolBars->toolBar1;
    wxToolBar* toolBar2 = m_toolBars->toolBar2;
    wxToolBar* toolBar3 = m_toolBars->toolBar3;

    // ���ù�������С
    toolBar1->SetSizeHints(-1, 28);
    toolBar2->SetSizeHints(-1, 28);
    toolBar3->SetSizeHints(-1, 28);

    // ȷ����������ʵ��
    toolBar1->Realize();
    toolBar2->Realize();
    toolBar3->Realize();

    // ʹ��AUI��������ӹ�����
    m_auiMgr.AddPane(toolBar1, wxAuiPaneInfo()
        .Name("Toolbar1")
        .Caption("Tools")
        .ToolbarPane()
        .Top()
        .LeftDockable(false)
        .RightDockable(false)
        .BottomDockable(false)
        .Gripper(false)
        .CloseButton(false)
        .PaneBorder(false)
        .Resizable(false)
        .BestSize(10000, 28));

    m_auiMgr.AddPane(toolBar2, wxAuiPaneInfo()
        .Name("Toolbar2")
        .Caption("Navigation")
        .ToolbarPane()
        .Top()
        .Row(1)  // �ڶ���
        .LeftDockable(false)
        .RightDockable(false)
        .BottomDockable(false)
        .Gripper(false)
        .CloseButton(false)
        .PaneBorder(false)
        .Resizable(false)
        .BestSize(10000, 28));

    m_auiMgr.AddPane(toolBar3, wxAuiPaneInfo()
        .Name("Toolbar3")
        .Caption("Actions")
        .ToolbarPane()
        .Top()
        .Row(2)  // ������
        .LeftDockable(false)
        .RightDockable(false)
        .BottomDockable(false)
        .Gripper(false)
        .CloseButton(false)
        .PaneBorder(false)
        .Resizable(false)
        .BestSize(10000, 28));
	m_toolBars->ChoosePageOne_toolBar1(-1); // ��ʼ��������״̬
	m_toolBars->ChoosePageOne_toolBar3(-1); // ��ʼ��������״̬
}