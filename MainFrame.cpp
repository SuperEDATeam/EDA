#include "MainFrame.h"
#include "MainMenuBar.h"
#include "CanvasPanel.h"
#include "PropertyPanel.h"
#include <wx/msgdlg.h>
#include "ToolboxPanel.h"   // 你的侧边栏
#include <wx/aui/aui.h>
#include "CanvasModel.h"
#include "my_log.h"
#include <wx/filename.h> 
#include <wx/sstream.h>

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

    /* 菜单 & 状态栏 */
    SetMenuBar(new MainMenuBar(this));
    CreateStatusBar(1);
    SetStatusText("Ready");
    SetTitle("Untitled");
    static_cast<MainMenuBar*>(GetMenuBar())->SetCurrentDocInWindowList("Untitled");

    /* 把窗口交给 AUI 管理（必须最先） */
    m_auiMgr.SetManagedWindow(this);

    /* 创建中央画布（先空白占位） */
    m_canvas = new CanvasPanel(this);
    m_canvas->SetBackgroundColour(*wxWHITE);

    // 工具栏
    m_toolBars = new ToolBars(this);
    AddToolBarsToAuiManager();

    // 工具管理器
    m_toolManager = new ToolManager(this, m_toolBars, m_canvas);
	m_toolManager->SetCurrentTool(ToolType::DRAG_TOOL);

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
    m_auiMgr.AddPane(m_canvas, wxAuiPaneInfo()
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
//    clone.SetPos(wxPoint(100, 100));   // 先放中央，后续用鼠标坐标
//    m_canvas->AddElement(clone);        
//}

void MainFrame::OnToolboxElement(wxCommandEvent& evt)
{
    wxString componentName = evt.GetString();

    if (m_toolManager) {
        m_toolManager->SetCurrentComponent(componentName);
    }
    else {
        wxLogError("ToolManager is not initialized!");
    }
}

//目前只修改了Mainframe。上面的私有变量，方法，以及json头文件
// 修改.h文件GenerateFileContent()的申明
//TODO 9.24
//只是打开一个新窗口，不会对现有窗口做任何改变
void MainFrame::DoFileNew() {
    // 直接创建一个新的空白MainFrame窗口
    MainFrame* newFrame = new MainFrame();  // 假设MainFrame构造函数会初始化空白状态

    // 显示新窗口（居中显示）
    newFrame->Centre(wxBOTH);
    newFrame->Show(true);

    // （可选）如果需要记录所有打开的窗口，可添加到窗口列表中
    // m_allFrames.push_back(newFrame);  // 需要在MainFrame类中声明m_allFrames
}









//保存文件的实现，包含后面四个方法
void MainFrame::DoFileSave() {
    // 1. 如果当前文档没有路径（未保存过），则调用"另存为"
    if (m_currentFilePath.IsEmpty()) {
        // 调用DoFileSaveAs()处理首次保存（需实现该方法）
        DoFileSaveAs();
        return;
    }

    // 2. 尝试将当前文档内容写入文件
    bool saveSuccess = SaveToFile(m_currentFilePath);

    // 3. 根据保存结果更新状态
    if (saveSuccess) {
        m_isModified = false;  // 保存成功，标记为未修改
        //UpdateTitle();         // 更新窗口标题（移除"*"等修改标记）
        SetStatusText(wxString::Format("已保存: %s", m_currentFilePath));
    }
    else {
        wxMessageBox(
            wxString::Format("保存失败: %s", m_currentFilePath),
            "错误",
            wxOK | wxICON_ERROR,
            this
        );
    }
}

// 辅助方法：将当前文档内容写入指定路径（修改为XML格式）
bool MainFrame::SaveToFile(const wxString& filePath) {
    // 1. 生成XML内容
    wxString xmlContent = GenerateFileContent();
    if (xmlContent.IsEmpty()) {
        return false;
    }

    // 2. 检查文件是否存在，不存在则创建
    if (!wxFile::Exists(filePath)) {
        wxFile tempFile;
        if (!tempFile.Open(filePath, wxFile::write)) {
            return false;
        }
        tempFile.Close();
    }

    // 3. 打开文件并写入XML内容
    wxFile file;
    if (!file.Open(filePath, wxFile::write)) {
        return false;
    }

    // 4. 写入并关闭文件
    size_t bytesWritten = file.Write(xmlContent);
    file.Close();

    // 5. 验证写入结果
    return bytesWritten == xmlContent.Length();
}








wxString MainFrame::GenerateFileContent()
{
    // 1. 创建XML文档
    wxXmlDocument doc;

    // 2. 根节点 <project>
    wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "project");
    root->AddAttribute("source", "2.7.1");
    root->AddAttribute("version", "1.0");
    doc.SetRoot(root);

    // 3. 注释
    root->AddChild(new wxXmlNode(wxXML_COMMENT_NODE,
        "This file is intended to be loaded by Logisim "
        "(http://www.cburch.com/logisim/)"));

    // 4. 库信息
    AddLibraryNode(root, "0", "#Wiring");
    AddLibraryNode(root, "1", "#Gates");

    // 5. 电路节点
    wxXmlNode* circuit = new wxXmlNode(wxXML_ELEMENT_NODE, "circuit");
    circuit->AddAttribute("name", "main");
    root->AddChild(circuit);

    // 保存元件信息（移除旋转角度相关代码）
    for (const auto& elem : m_canvas->GetElements()) {
        wxXmlNode* element = new wxXmlNode(wxXML_ELEMENT_NODE, "element");
        element->AddAttribute("name", elem.GetName());  // 保留元件名称
        element->AddAttribute("x", wxString::Format("%d", elem.GetPos().x));  // 保留X坐标
        element->AddAttribute("y", wxString::Format("%d", elem.GetPos().y));  // 保留Y坐标
        // 移除下面这行关于rotation的代码
        // element->AddAttribute("rotation", wxString::Format("%d", elem.GetRotation()));
        circuit->AddChild(element);
    }

    // 7. 保存连线信息
    for (const auto& wire : m_canvas->GetWires()) {
        // 直接访问Wire类的pts成员变量获取点集合
        const auto& pts = wire.pts;  // 关键修改：使用wire.pts替代wire.GetPoints()
        if (pts.size() < 2) continue;

        wxXmlNode* wireNode = new wxXmlNode(wxXML_ELEMENT_NODE, "wire");
        // 保存起点（第一个点）和终点（最后一个点）
        wireNode->AddAttribute("from", wxString::Format("(%d,%d)", pts[0].pos.x, pts[0].pos.y));
        wireNode->AddAttribute("to", wxString::Format("(%d,%d)", pts.back().pos.x, pts.back().pos.y));

        // 保存中间点（若存在）
        if (pts.size() > 2) {
            wxString midPoints;
            for (size_t i = 1; i < pts.size() - 1; ++i) {
                midPoints += wxString::Format("(%d,%d);", pts[i].pos.x, pts[i].pos.y);
            }
            wireNode->AddAttribute("midpoints", midPoints);
        }
        circuit->AddChild(wireNode);
    }

    // 8. 输出XML内容
    wxStringOutputStream strStream;
    doc.Save(strStream, wxXML_DOCUMENT_TYPE_NODE);
    return strStream.GetString();
}









void MainFrame::DoFileOpen(const wxString& path)
{
    wxString filePath = path;

    // 如果没有提供路径，显示文件选择对话框
    if (filePath.IsEmpty()) {
        wxFileDialog openDialog(
            this,
            "打开电路文件",
            "",
            "",
            "电路文件 (*.circ)|*.circ|所有文件 (*.*)|*.*",
            wxFD_OPEN | wxFD_FILE_MUST_EXIST
        );

        if (openDialog.ShowModal() != wxID_OK) {
            return;
        }
        filePath = openDialog.GetPath();
    }

    // 尝试读取文件内容
    wxFile file;
    if (!file.Open(filePath, wxFile::read)) {
        wxMessageBox("无法打开文件: " + filePath, "错误", wxOK | wxICON_ERROR);
        return;
    }

    // 读取XML内容
    wxString xmlContent;
    file.ReadAll(&xmlContent);
    file.Close();

    // 解析XML
    wxXmlDocument doc;
    wxStringInputStream stream(xmlContent);
    if (!doc.Load(stream)) {
        wxMessageBox("文件格式错误: " + filePath, "错误", wxOK | wxICON_ERROR);
        return;
    }

    // 清空当前画布
    m_canvas->ClearAll();

    // 解析根节点
    wxXmlNode* root = doc.GetRoot();
    if (!root || root->GetName() != "project") {
        wxMessageBox("无效的电路文件", "错误", wxOK | wxICON_ERROR);
        return;
    }

    // 查找电路节点
    wxXmlNode* circuit = root->GetChildren();
    while (circuit) {
        if (circuit->GetName() == "circuit") {
            break;
        }
        circuit = circuit->GetNext();
    }

    if (!circuit) {
        wxMessageBox("文件中未找到电路信息", "错误", wxOK | wxICON_ERROR);
        return;
    }

    // 解析元件和连线
    wxXmlNode* child = circuit->GetChildren();
    while (child) {
        // 解析元件（移除旋转角度相关代码）
        if (child->GetName() == "element") {
            wxString name = child->GetAttribute("name");
            int x = wxAtoi(child->GetAttribute("x", "0"));
            int y = wxAtoi(child->GetAttribute("y", "0"));
            // 移除下面这行关于rotation的读取
            // int rotation = wxAtoi(child->GetAttribute("rotation", "0"));

            m_canvas->PlaceElement(name, wxPoint(x, y));
            // 同时移除设置旋转角度的逻辑（如果有的话）
        }

        // 解析连线（在DoFileOpen方法中）
        else if (child->GetName() == "wire") {
            wxString fromStr = child->GetAttribute("from");
            wxString toStr = child->GetAttribute("to");
            wxString midPointsStr = child->GetAttribute("midpoints", "");

            // 解析坐标点 (x,y)
            auto parsePoint = [](const wxString& str) -> wxPoint {
                int x = 0, y = 0;
                if (sscanf(str.ToUTF8().data(), "(%d,%d)", &x, &y) == 2) {
                    return wxPoint(x, y);
                }
                return wxPoint(0, 0);
                };

            // 重建pts集合
            std::vector<ControlPoint> pts;
            pts.push_back({ parsePoint(fromStr), CPType::Pin });  // 起点（Pin类型）

            // 解析中间点
            if (!midPointsStr.IsEmpty()) {
                wxArrayString midPoints = wxSplit(midPointsStr, ';');
                for (const auto& ptStr : midPoints) {
                    if (ptStr.IsEmpty()) continue;
                    pts.push_back({ parsePoint(ptStr), CPType::Bend });  // 中间点为折点
                }
            }

            pts.push_back({ parsePoint(toStr), CPType::Free });  // 终点（Free类型）

            // 创建Wire并添加到画布
            Wire wire;
            wire.pts = pts;  // 直接赋值给Wire的pts成员
            wire.GenerateCells();  // 生成网格点（保持显示一致性）
            m_canvas->AddWire(wire);
        }
        
        child = child->GetNext();
    }

    // 更新状态
    m_currentFilePath = filePath;
    m_isModified = false;
    SetTitle(wxFileName(filePath).GetFullName());
    static_cast<MainMenuBar*>(GetMenuBar())->AddFileToHistory(filePath);
    SetStatusText("已打开: " + filePath);
}











// 辅助方法：添加元件库节点
void MainFrame::AddLibraryNode(wxXmlNode* parent, const wxString& name, const wxString& desc) {
    wxXmlNode* lib = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("lib"));
    lib->AddAttribute(wxT("name"), name);
    lib->AddAttribute(wxT("desc"), desc);
    parent->AddChild(lib);
}

// 辅助方法：添加导线节点
void MainFrame::AddWireNode(wxXmlNode* parent, const wxString& from, const wxString& to) {
    wxXmlNode* wire = new wxXmlNode(wxXML_ELEMENT_NODE, wxT("wire"));
    wire->AddAttribute(wxT("from"), from);
    wire->AddAttribute(wxT("to"), to);
    parent->AddChild(wire);
}
// 需配套实现"另存为"方法（处理首次保存）
void MainFrame::DoFileSaveAs() {
    // 弹出文件选择对话框
    wxFileDialog saveDialog(
        this,
        "另存为",
        "",
        "Untitled.circ",  // 默认文件名
        "电路文件 (*.circ)|*.circ|所有文件 (*.*)|*.*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT  // 提示覆盖已有文件
    );

    // 用户取消则返回
    if (saveDialog.ShowModal() != wxID_OK) {
        return;
    }

    // 获取用户选择的路径
    wxString newPath = saveDialog.GetPath();
    m_currentFilePath = newPath;

    // 执行保存
    DoFileSave();

    // （可选）添加到最近文件历史
    static_cast<MainMenuBar*>(GetMenuBar())->AddFileToHistory(newPath);
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

void MainFrame::AddToolBarsToAuiManager() {
    if (!m_toolBars) return;

    wxToolBar* toolBar1 = m_toolBars->toolBar1;
    wxToolBar* toolBar2 = m_toolBars->toolBar2;
    wxToolBar* toolBar3 = m_toolBars->toolBar3;

    // 设置工具栏大小
    toolBar1->SetSizeHints(-1, 28);
    toolBar2->SetSizeHints(-1, 28);
    toolBar3->SetSizeHints(-1, 28);

    // 确保工具栏已实现
    toolBar1->Realize();
    toolBar2->Realize();
    toolBar3->Realize();

    // 使用AUI管理器添加工具栏
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
        .Row(1)  // 第二行
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
        .Row(2)  // 第三行
        .LeftDockable(false)
        .RightDockable(false)
        .BottomDockable(false)
        .Gripper(false)
        .CloseButton(false)
        .PaneBorder(false)
        .Resizable(false)
        .BestSize(10000, 28));
    m_toolBars->ChoosePageOne_toolBar1(-1); // 初始化工具栏状态
    m_toolBars->ChoosePageOne_toolBar3(-1); // 初始化工具栏状态
}
