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
#include <wx/dir.h>
#include <wx/image.h>
#include <map>
#include <wx/arrstr.h>


// 你的图标绝对路径（已确认正确，不用改）
#define ICON_FOLDER wxT("E:/课程/综设/MyLogisim/icons/")

static void MY_LOG(const wxString& s)
{
    wxFile f(wxStandardPaths::Get().GetTempDir() + "\\logsim_tools.log",
        wxFile::write_append);
    if (f.IsOpened()) {
        f.Write(wxDateTime::Now().FormatISOCombined() + "  " + s + "\n");
        f.Close();
    }
}

// 自定义树项数据（不变）
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
//EVT_TREE_SEL_CHANGED(wxID_ANY, ToolboxPanel::OnToolSelected)
wxEND_EVENT_TABLE()


ToolboxPanel::ToolboxPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1))
{
    // 添加 PNG 图像处理器初始化
    wxImage::AddHandler(new wxPNGHandler);

    /* 1. 布局调整：垂直排列“工具树”和“属性面板” */
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // 1.1 初始化工具树（原有逻辑保留，调整比例）
    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_FULL_ROW_HIGHLIGHT);
    mainSizer->Add(m_tree, 7, wxEXPAND | wxALL, 0); // 工具树占70%高度

    // 初始化属性面板 - 使用兼容的 API
    m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER);
    m_propGrid->SetCaptionBackgroundColour(wxColour(230, 230, 230));

    // 在 wxWidgets 3.2.2.1 中设置列标题的正确方式
    // 移除不兼容的 SetColumnLabel 调用
    // m_propGrid->SetColumnLabel(0, "Property");
    // m_propGrid->SetColumnLabel(1, "Value");

    mainSizer->Add(m_propGrid, 3, wxEXPAND | wxALL, 0);

    SetSizer(mainSizer);

    /* 2. 初始化图像列表 + 加载图标（原有逻辑完全保留） */
    m_imgList = new wxImageList(24, 24, true, 500);
    m_imgList->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(24, 24))); // 0: 文件夹图标
    // -------------------------- 原有图标加载代码完全保留 --------------------------
    // -------------------------- 原有图标加载代码完全保留（补全中间省略的部分） --------------------------
    LoadToolIcon("Wire", "wiring.png");               // 1: Wiring-Wire
    LoadToolIcon("Splitter", "splitter.png");         // 2: Wiring-Splitter
    // 【补全开始：之前被省略的中间工具加载代码】
    LoadToolIcon("Pin (Input)", "pinInput.png");      // 3: Wiring-Pin (Input)
    LoadToolIcon("Pin (Output)", "pinOutput.png");    // 4: Wiring-Pin (Output)
    LoadToolIcon("Probe", "probe.png");               // 5: Wiring-Probe
    LoadToolIcon("Tunnel", "tunnel.png");             // 6: Wiring-Tunnel
    LoadToolIcon("Pull Resistor", "pullrect.png");    // 7: Wiring-Pull Resistor
    LoadToolIcon("Clock", "clock.png");               // 8: Wiring-Clock
    LoadToolIcon("Constant", "constant.png");         // 9: Wiring-Constant
    LoadToolIcon("Power", "power.png");               // 10: Wiring-Power
    LoadToolIcon("Ground", "ground.png");             // 11: Wiring-Ground
    LoadToolIcon("Transmission Gate", "transmis.png");// 12: Wiring-Transmission Gate
    LoadToolIcon("Bit Extender", "extender.png");     // 13: Wiring-Bit Extender

    // 2. Gates 分类（逻辑门）
    LoadToolIcon("Buffer Gate", "bufferGate.png");    // 14: Gates-Buffer Gate
    LoadToolIcon("AND Gate", "andGate.png");          // 15: Gates-AND Gate
    LoadToolIcon("AND Gate (Rect)", "andGateRect.png");// 16: Gates-AND Gate(Rect)
    LoadToolIcon("NAND Gate", "nandGate.png");        // 17: Gates-NAND Gate
    LoadToolIcon("NAND Gate (Rect)", "nandGateRect.png");// 18: Gates-NAND Gate(Rect)
    LoadToolIcon("OR Gate", "orGate.png");            // 19: Gates-OR Gate
    LoadToolIcon("OR Gate (Rect)", "orGateRect.png"); // 20: Gates-OR Gate(Rect)
    LoadToolIcon("NOR Gate", "norGate.png");          // 21: Gates-NOR Gate
    LoadToolIcon("NOR Gate (Rect)", "norGateRect.png");// 22: Gates-NOR Gate(Rect)
    LoadToolIcon("XOR Gate", "xorGate.png");          // 23: Gates-XOR Gate
    LoadToolIcon("XOR Gate (Rect)", "xorGateRect.png");// 24: Gates-XOR Gate(Rect)
    LoadToolIcon("XNOR Gate", "xnorGate.png");        // 25: Gates-XNOR Gate
    LoadToolIcon("XNOR Gate (Rect)", "xnorGateRect.png");// 26: Gates-XNOR Gate(Rect)
    LoadToolIcon("Odd Parity Gate", "parityOddGate.png");// 27: Gates-Odd Parity Gate
    LoadToolIcon("Even Parity Gate", "parityEvenGate.png");// 28: Gates-Even Parity Gate
    LoadToolIcon("Controlled Buffer", "controlledBuffer.png");// 29: Gates-Controlled Buffer
    LoadToolIcon("Controlled Inverter", "controlledInverter.png");// 30: Gates-Controlled Inverter

    // 3. Plexers 分类
    LoadToolIcon("Multiplexer", "multiplexer.png");   // 31: Plexers-Multiplexer
    LoadToolIcon("Demultiplexer", "demultiplexer.png");// 32: Plexers-Demultiplexer
    LoadToolIcon("Decoder", "decoder.png");           // 33: Plexers-Decoder
    LoadToolIcon("Priority Encoder", "priencod.png"); // 34: Plexers-Priority Encoder
    LoadToolIcon("Bit Selector", "bitSelector.png");  // 35: Plexers-Bit Selector

    // 4. Arithmetic 分类
    LoadToolIcon("Adder", "adder.png");               // 36: Arithmetic-Adder
    LoadToolIcon("Subtractor", "subtractor.png");     // 37: Arithmetic-Subtractor
    LoadToolIcon("Multiplier", "multiplier.png");     // 38: Arithmetic-Multiplier
    LoadToolIcon("Divider", "divider.png");           // 39: Arithmetic-Divider
    LoadToolIcon("Negator", "negator.png");           // 40: Arithmetic-Negator
    LoadToolIcon("Comparator", "comparator.png");     // 41: Arithmetic-Comparator
    LoadToolIcon("Shifter", "shifter.png");           // 42: Arithmetic-Shifter
    LoadToolIcon("Bit Adder", "bitadder.png");        // 43: Arithmetic-Bit Adder
    LoadToolIcon("Bit Finder", "bitfindr.png");       // 44: Arithmetic-Bit Finder

    // 5. Memory 分类
    LoadToolIcon("D Flip-Flop", "dFlipFlop.png");     // 45: Memory-D Flip-Flop
    LoadToolIcon("T Flip-Flop", "tFlipFlop.png");     // 46: Memory-T Flip-Flop
    LoadToolIcon("JK Flip-Flop", "jkFlipFlop.png");   // 47: Memory-JK Flip-Flop
    LoadToolIcon("SR Flip-Flop", "srFlipFlop.png");   // 48: Memory-SR Flip-Flop
    LoadToolIcon("Register", "register.png");         // 49: Memory-Register
    LoadToolIcon("Counter", "counter.png");           // 50: Memory-Counter
    LoadToolIcon("Shift Register", "shiftreg.png");   // 51: Memory-Shift Register
    LoadToolIcon("Random Generator", "random.png");   // 52: Memory-Random Generator
    LoadToolIcon("RAM", "ram.png");                   // 53: Memory-RAM
    LoadToolIcon("ROM", "rom.png");                   // 54: Memory-ROM

    // 6. Input/Output 分类
    LoadToolIcon("Button", "button.png");             // 55: Input/Output-Button
    LoadToolIcon("Joystick", "joystick.png");         // 56: Input/Output-Joystick
    LoadToolIcon("Keyboard", "keyboard.png");         // 57: Input/Output-Keyboard
    LoadToolIcon("LED", "led.png");                   // 58: Input/Output-LED
    LoadToolIcon("7-Segment Display", "7seg.png");    // 59: Input/Output-7-Segment Display
    LoadToolIcon("Hex Digit Display", "hexdig.png");  // 60: Input/Output-Hex Digit Display
    LoadToolIcon("LED Matrix", "dotmat.png");         // 61: Input/Output-LED Matrix
    LoadToolIcon("TTY", "tty.png");                   // 62: Input/Output-TTY

    // 7. Tools 分类（Label Tool 是最后一个）
    LoadToolIcon("Poke Tool", "poke.png");            // 63: Tools-Poke Tool
    LoadToolIcon("Edit Tool", "select.png");          // 64: Tools-Edit Tool
    LoadToolIcon("Select Tool", "select.png");        // 65: Tools-Select Tool
    LoadToolIcon("Wiring Tool", "wiring.png");        // 66: Tools-Wiring Tool
    LoadToolIcon("Text Tool", "text.png");            // 67: Tools-Text Tool
    LoadToolIcon("Menu Tool", "menu.png");            // 68: Tools-Menu Tool
    LoadToolIcon("Label Tool", "text.png");          // 69: Tools-Label Tool
    // 【补全结束】
    m_tree->AssignImageList(m_imgList);

    MY_LOG("✅ 图像列表已关联到工具树，图像数量：" + wxString::Format("%d", m_imgList->GetImageCount()));

    /* 3. 字体样式（原有逻辑保留） */
    wxFont font(wxFontInfo(11).FaceName("Segoe UI"));
    m_tree->SetFont(font);
    m_tree->SetIndent(20);
    m_propGrid->SetFont(font); // 给属性面板设置相同字体

    /* 4. 新增：初始化工具-属性映射表 + 绑定选中事件 */
    InitToolPropertyMap();
    Rebuild(); // 构建工具树

    // 绑定“树节点选中事件”（Logisim是单击选中，不是双击）
    m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &ToolboxPanel::OnToolSelected, this);
}

void ToolboxPanel::LoadToolIcon(const wxString& toolName, const wxString& pngFileName)
{
    wxString fullPath = ICON_FOLDER + pngFileName;
    MY_LOG("🔍 开始加载图标：" + toolName + " → 文件路径：" + fullPath);

    // 加载PNG并检查是否有效
    wxBitmap icon(fullPath, wxBITMAP_TYPE_PNG);
    bool isLoaded = icon.IsOk();

    if (isLoaded) {
        int iconIndex = m_imgList->Add(icon); // 添加到图像列表
        MY_LOG("✅ 加载成功：" + toolName + " → 图标索引：" + wxString::Format("%d", iconIndex) + "，图像列表当前数量：" + wxString::Format("%d", m_imgList->GetImageCount()));
    }
    else {
        MY_LOG("❌ 加载失败：" + toolName + " → 文件不存在/损坏/格式错误");
    }
}

// -------------------------- 核心：按Logisim分类构建侧边栏 --------------------------
void ToolboxPanel::Rebuild()
{
    m_tree->DeleteAllItems();
    wxTreeItemId root = m_tree->AddRoot("Logisim Tools", 0, 0); // 根节点（隐藏）

    // ================================= 1. Wiring 分类（Logisim 原生顺序） =================================
    wxArrayString wiringTools;
    wiringTools.Add("Wire");
    wiringTools.Add("Splitter");
    wiringTools.Add("Pin (Input)");
    wiringTools.Add("Pin (Output)");
    wiringTools.Add("Probe");
    wiringTools.Add("Tunnel");
    wiringTools.Add("Pull Resistor");
    wiringTools.Add("Clock");
    wiringTools.Add("Constant");
    wiringTools.Add("Power");
    wiringTools.Add("Ground");
    wiringTools.Add("Transmission Gate");
    wiringTools.Add("Bit Extender");
    wxTreeItemId wiringId = m_tree->AppendItem(root, "Wiring", 0, 0);
    m_tree->SetItemBold(wiringId, true);
    for (size_t i = 0; i < wiringTools.GetCount(); i++) {
        wxString tool = wiringTools[i];
        int iconIdx = GetToolIconIndex(tool);
        m_tree->AppendItem(wiringId, tool, iconIdx, iconIdx, new wxStringTreeItemData(tool));
    }

    // ================================= 2. Gates 分类（Logisim 原生顺序） =================================
    wxArrayString gatesTools;
    gatesTools.Add("Buffer Gate");
    gatesTools.Add("AND Gate");
    gatesTools.Add("AND Gate (Rect)");
    gatesTools.Add("NAND Gate");
    gatesTools.Add("NAND Gate (Rect)");
    gatesTools.Add("OR Gate");
    gatesTools.Add("OR Gate (Rect)");
    gatesTools.Add("NOR Gate");
    gatesTools.Add("NOR Gate (Rect)");
    gatesTools.Add("XOR Gate");
    gatesTools.Add("XOR Gate (Rect)");
    gatesTools.Add("XNOR Gate");
    gatesTools.Add("XNOR Gate (Rect)");
    gatesTools.Add("Odd Parity Gate");
    gatesTools.Add("Even Parity Gate");
    gatesTools.Add("Controlled Buffer");
    gatesTools.Add("Controlled Inverter");
    wxTreeItemId gatesId = m_tree->AppendItem(root, "Gates", 0, 0);
    m_tree->SetItemBold(gatesId, true);
    for (size_t i = 0; i < gatesTools.GetCount(); i++) {
        wxString tool = gatesTools[i];
        int iconIdx = GetToolIconIndex(tool);
        m_tree->AppendItem(gatesId, tool, iconIdx, iconIdx, new wxStringTreeItemData(tool));
    }

    // ================================= 3. Plexers 分类（原 Combinational，重命名贴合 Logisim） =================================
    wxArrayString plexersTools;
    plexersTools.Add("Multiplexer");
    plexersTools.Add("Demultiplexer");
    plexersTools.Add("Decoder");
    plexersTools.Add("Priority Encoder");
    plexersTools.Add("Bit Selector");
    wxTreeItemId plexersId = m_tree->AppendItem(root, "Plexers", 0, 0);
    m_tree->SetItemBold(plexersId, true);
    for (size_t i = 0; i < plexersTools.GetCount(); i++) {
        wxString tool = plexersTools[i];
        int iconIdx = GetToolIconIndex(tool);
        m_tree->AppendItem(plexersId, tool, iconIdx, iconIdx, new wxStringTreeItemData(tool));
    }

    // ================================= 4. Arithmetic 分类（补充缺失工具） =================================
    wxArrayString arithmeticTools;
    arithmeticTools.Add("Adder");
    arithmeticTools.Add("Subtractor");
    arithmeticTools.Add("Multiplier");
    arithmeticTools.Add("Divider");
    arithmeticTools.Add("Negator");
    arithmeticTools.Add("Comparator");
    arithmeticTools.Add("Shifter");
    arithmeticTools.Add("Bit Adder");
    arithmeticTools.Add("Bit Finder");
    wxTreeItemId arithmeticId = m_tree->AppendItem(root, "Arithmetic", 0, 0);
    m_tree->SetItemBold(arithmeticId, true);
    for (size_t i = 0; i < arithmeticTools.GetCount(); i++) {
        wxString tool = arithmeticTools[i];
        int iconIdx = GetToolIconIndex(tool);
        m_tree->AppendItem(arithmeticId, tool, iconIdx, iconIdx, new wxStringTreeItemData(tool));
    }

    // ================================= 5. Memory 分类（合并 Sequential+Storage，Logisim 原生） =================================
    wxArrayString memoryTools;
    memoryTools.Add("D Flip-Flop");
    memoryTools.Add("T Flip-Flop");
    memoryTools.Add("JK Flip-Flop");
    memoryTools.Add("SR Flip-Flop");
    memoryTools.Add("Register");
    memoryTools.Add("Counter");
    memoryTools.Add("Shift Register");
    memoryTools.Add("Random Generator");
    memoryTools.Add("RAM");
    memoryTools.Add("ROM");
    wxTreeItemId memoryId = m_tree->AppendItem(root, "Memory", 0, 0);
    m_tree->SetItemBold(memoryId, true);
    for (size_t i = 0; i < memoryTools.GetCount(); i++) {
        wxString tool = memoryTools[i];
        int iconIdx = GetToolIconIndex(tool);
        m_tree->AppendItem(memoryId, tool, iconIdx, iconIdx, new wxStringTreeItemData(tool));
    }

    // ================================= 6. Input/Output 分类（补充缺失设备） =================================
    wxArrayString ioTools;
    ioTools.Add("Button");
    ioTools.Add("Joystick");
    ioTools.Add("Keyboard");
    ioTools.Add("LED");
    ioTools.Add("7-Segment Display");
    ioTools.Add("Hex Digit Display");
    ioTools.Add("LED Matrix");
    ioTools.Add("TTY");
    wxTreeItemId ioId = m_tree->AppendItem(root, "Input/Output", 0, 0);
    m_tree->SetItemBold(ioId, true);
    for (size_t i = 0; i < ioTools.GetCount(); i++) {
        wxString tool = ioTools[i];
        int iconIdx = GetToolIconIndex(tool);
        m_tree->AppendItem(ioId, tool, iconIdx, iconIdx, new wxStringTreeItemData(tool));
    }

    // ================================= 7. Tools 分类（新增！Logisim 操作工具） =================================
    wxArrayString toolsTools;
    toolsTools.Add("Poke Tool");
    toolsTools.Add("Edit Tool");
    toolsTools.Add("Select Tool");
    toolsTools.Add("Wiring Tool");
    toolsTools.Add("Text Tool");
    toolsTools.Add("Menu Tool");
    toolsTools.Add("Label Tool");
    wxTreeItemId toolsId = m_tree->AppendItem(root, "Tools", 0, 0);
    m_tree->SetItemBold(toolsId, true);
    for (size_t i = 0; i < toolsTools.GetCount(); i++) {
        wxString tool = toolsTools[i];
        int iconIdx = GetToolIconIndex(tool);
        m_tree->AppendItem(toolsId, tool, iconIdx, iconIdx, new wxStringTreeItemData(tool));
    }

    // 展开所有分类子节点（保持原有逻辑）
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(root, cookie);
    while (child.IsOk()) {
        m_tree->Expand(child);
        child = m_tree->GetNextChild(root, cookie);
    }
}

// -------------------------- 工具名 → 图标索引映射（必须和加载顺序一致） --------------------------
int ToolboxPanel::GetToolIconIndex(const wxString& toolName)
{
    static std::map<wxString, int> toolToIconMap = {
        // 1. Wiring 分类（索引1-13）
        {"Wire", 1},
        {"Splitter", 2},
        {"Pin (Input)", 3},
        {"Pin (Output)", 4},
        {"Probe", 5},
        {"Tunnel", 6},
        {"Pull Resistor", 7},
        {"Clock", 8},
        {"Constant", 9},
        {"Power", 10},
        {"Ground", 11},
        {"Transmission Gate", 12},
        {"Bit Extender", 13},

        // 2. Gates 分类（索引14-30）
        {"Buffer Gate", 14},
        {"AND Gate", 15},
        {"AND Gate (Rect)", 16},
        {"NAND Gate", 17},
        {"NAND Gate (Rect)", 18},
        {"OR Gate", 19},
        {"OR Gate (Rect)", 20},
        {"NOR Gate", 21},
        {"NOR Gate (Rect)", 22},
        {"XOR Gate", 23},
        {"XOR Gate (Rect)", 24},
        {"XNOR Gate", 25},
        {"XNOR Gate (Rect)", 26},
        {"Odd Parity Gate", 27},
        {"Even Parity Gate", 28},
        {"Controlled Buffer", 29},
        {"Controlled Inverter", 30},

        // 3. Plexers 分类（索引31-35）
        {"Multiplexer", 31},
        {"Demultiplexer", 32},
        {"Decoder", 33},
        {"Priority Encoder", 34},
        {"Bit Selector", 35},

        // 4. Arithmetic 分类（索引36-44）
        {"Adder", 36},
        {"Subtractor", 37},
        {"Multiplier", 38},
        {"Divider", 39},
        {"Negator", 40},
        {"Comparator", 41},
        {"Shifter", 42},
        {"Bit Adder", 43},
        {"Bit Finder", 44},

        // 5. Memory 分类（索引45-54）
        {"D Flip-Flop", 45},
        {"T Flip-Flop", 46},
        {"JK Flip-Flop", 47},
        {"SR Flip-Flop", 48},
        {"Register", 49},
        {"Counter", 50},
        {"Shift Register", 51},
        {"Random Generator", 52},
        {"RAM", 53},
        {"ROM", 54},

        // 6. Input/Output 分类（索引55-62）
        {"Button", 55},
        {"Joystick", 56},
        {"Keyboard", 57},
        {"LED", 58},
        {"7-Segment Display", 59},
        {"Hex Digit Display", 60},
        {"LED Matrix", 61},
        {"TTY", 62},

        // 7. Tools 分类（索引63-69）
        {"Poke Tool", 63},
        {"Edit Tool", 64},
        {"Select Tool", 65},
        {"Wiring Tool", 66},
        {"Text Tool", 67},
        {"Menu Tool", 68},
        {"Label Tool", 69}
    };
    // 找不到对应工具时，默认用Wire的图标（避免空白）
    auto it = toolToIconMap.find(toolName);
    return (it != toolToIconMap.end()) ? it->second : 1;
}

// 以下两个函数不变（激活和拖拽功能）
void ToolboxPanel::OnItemActivated(wxTreeEvent& evt)
{
    wxString name = m_tree->GetItemText(evt.GetItem());
    wxCommandEvent cmdEvt(wxEVT_COMMAND_MENU_SELECTED, wxID_HIGHEST + 900);
    cmdEvt.SetString(name);
    wxPostEvent(GetParent(), cmdEvt);
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

void ToolboxPanel::InitToolPropertyMap()
{
    // -------------------------- 1. Pin (Input) 属性 --------------------------
    std::vector<ToolProperty> pinInputProps;
    pinInputProps.push_back(ToolProperty("Facing", "string", "East"));
    pinInputProps.push_back(ToolProperty("Output?", "bool", false));
    pinInputProps.push_back(ToolProperty("Data Bits", "int", 1L));
    pinInputProps.push_back(ToolProperty("Three-state?", "bool", true));
    pinInputProps.push_back(ToolProperty("Pull Behavior", "string", "Unchanged"));
    pinInputProps.push_back(ToolProperty("Label", "string", ""));
    pinInputProps.push_back(ToolProperty("Label Location", "string", "West"));
    pinInputProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    m_toolPropMap["Pin (Input)"] = pinInputProps;

    // -------------------------- 2. OR Gate 属性 --------------------------
    std::vector<ToolProperty> orGateProps;
    orGateProps.push_back(ToolProperty("Facing", "string", "East"));
    orGateProps.push_back(ToolProperty("Data Bits", "int", 1L));
    orGateProps.push_back(ToolProperty("Gate Size", "string", "Medium"));
    orGateProps.push_back(ToolProperty("Number Of Inputs", "int", 5L));
    orGateProps.push_back(ToolProperty("Output Value", "string", "0/1"));
    orGateProps.push_back(ToolProperty("Label", "string", ""));
    orGateProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    orGateProps.push_back(ToolProperty("Negate 1 (Top)", "bool", false));
    orGateProps.push_back(ToolProperty("Negate 2", "bool", false));
    orGateProps.push_back(ToolProperty("Negate 3", "bool", false));
    orGateProps.push_back(ToolProperty("Negate 4", "bool", false));
    orGateProps.push_back(ToolProperty("Negate 5 (Bottom)", "bool", false));
    m_toolPropMap["OR Gate"] = orGateProps;

    // -------------------------- 3. Demultiplexer 属性 --------------------------
    std::vector<ToolProperty> demuxProps;
    demuxProps.push_back(ToolProperty("Facing", "string", "East"));
    demuxProps.push_back(ToolProperty("Select Location", "string", "Bottom/Left"));
    demuxProps.push_back(ToolProperty("Select Bits", "int", 1L));
    demuxProps.push_back(ToolProperty("Data Bits", "int", 1L));
    demuxProps.push_back(ToolProperty("Three-state?", "bool", false));
    demuxProps.push_back(ToolProperty("Disabled Output", "string", "Floating"));
    demuxProps.push_back(ToolProperty("Include Enable?", "bool", true));
    m_toolPropMap["Demultiplexer"] = demuxProps;

    // -------------------------- 4. Comparator 属性 --------------------------
    std::vector<ToolProperty> comparatorProps;
    comparatorProps.push_back(ToolProperty("Data Bits", "int", 8L));
    comparatorProps.push_back(ToolProperty("Numeric Type", "string", "2's Complement"));
    m_toolPropMap["Comparator"] = comparatorProps;

    // -------------------------- 5. S-R Flip-Flop 属性 --------------------------
    std::vector<ToolProperty> srFlipFlopProps;
    srFlipFlopProps.push_back(ToolProperty("Trigger", "string", "Rising Edge"));
    srFlipFlopProps.push_back(ToolProperty("Label", "string", ""));
    srFlipFlopProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    m_toolPropMap["SR Flip-Flop"] = srFlipFlopProps;

    // -------------------------- 6. LED 属性 --------------------------
    std::vector<ToolProperty> ledProps;
    // 对于颜色，使用字符串表示，然后在 UpdatePropertyPanel 中解析
    ledProps.push_back(ToolProperty("Facing", "string", "West"));
    ledProps.push_back(ToolProperty("On Color", "color", "#F00000")); // 使用十六进制字符串
    ledProps.push_back(ToolProperty("Off Color", "color", "#404040"));
    ledProps.push_back(ToolProperty("Active On High?", "bool", true));
    ledProps.push_back(ToolProperty("Label", "string", ""));
    ledProps.push_back(ToolProperty("Label Location", "string", "Center"));
    ledProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    ledProps.push_back(ToolProperty("Label Color", "color", "#000000"));
    m_toolPropMap["LED"] = ledProps;

    // -------------------------- 其他工具属性 --------------------------
    m_toolPropMap["Wire"] = std::vector<ToolProperty>();

    std::vector<ToolProperty> buttonProps;
    buttonProps.push_back(ToolProperty("Facing", "string", "North"));
    buttonProps.push_back(ToolProperty("Trigger", "string", "Edge"));
    buttonProps.push_back(ToolProperty("Label", "string", ""));
    buttonProps.push_back(ToolProperty("Label Location", "string", "South"));
    m_toolPropMap["Button"] = buttonProps;
}

void ToolboxPanel::OnToolSelected(wxTreeEvent& evt)
{
    // 获取选中节点的工具名（过滤分类节点，只处理工具节点）
    wxTreeItemId selectedItem = evt.GetItem();
    if (!m_tree->ItemHasChildren(selectedItem)) {
        wxString toolName = m_tree->GetItemText(selectedItem);
        UpdatePropertyPanel(toolName);
    }
    else {
        // 选中分类节点，清空属性面板
        m_propGrid->Clear();
        m_propGrid->Append(new wxPropertyCategory("No Tool Selected"));
    }
}

void ToolboxPanel::UpdatePropertyPanel(const wxString& toolName)
{
    // 1. 清空原有属性
    m_propGrid->Clear();

    // 2. 添加分类标题
    wxPropertyCategory* category = new wxPropertyCategory(wxString::Format("Tool: %s", toolName));
    m_propGrid->Append(category);

    // 3. 从映射表中获取当前工具的属性列表
    auto it = m_toolPropMap.find(toolName);
    if (it == m_toolPropMap.end()) {
        wxStringProperty* noProps = new wxStringProperty("Info", "Info", "No configurable properties");
        m_propGrid->Append(noProps);
        return;
    }

    // 4. 添加属性（只支持基本类型）
    std::vector<ToolProperty> props = it->second;
    for (auto& prop : props) {
        if (prop.propType == "bool") {
            wxBoolProperty* boolProp = new wxBoolProperty(prop.propName, prop.propName);
            boolProp->SetValue(prop.defaultValue.GetBool());
            m_propGrid->Append(boolProp);
        }
        else if (prop.propType == "int") {
            wxIntProperty* intProp = new wxIntProperty(prop.propName, prop.propName);
            intProp->SetValue(prop.defaultValue.GetLong());
            m_propGrid->Append(intProp);
        }
        else if (prop.propType == "string") {
            wxStringProperty* strProp = new wxStringProperty(prop.propName, prop.propName);
            strProp->SetValue(prop.defaultValue.GetString());
            m_propGrid->Append(strProp);
        }
        // 忽略颜色类型，或者将其转换为字符串
    }
}