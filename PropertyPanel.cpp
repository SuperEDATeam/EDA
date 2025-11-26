#include "PropertyPanel.h"

PropertyPanel::PropertyPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    // 创建垂直布局
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // 创建标题
    m_title = new wxStaticText(this, wxID_ANY, "No element selected");
    sizer->Add(m_title, 0, wxALL, 5);

    // 创建属性表格（新增）
    m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER);
    m_propGrid->SetCaptionBackgroundColour(wxColour(230, 230, 230));

    // 设置字体
    wxFont font(wxFontInfo(11).FaceName("Segoe UI"));
    m_propGrid->SetFont(font);

    sizer->Add(m_propGrid, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);

    // 初始化属性映射表
    InitToolPropertyMap();
}

void PropertyPanel::ShowElement(const wxString& name)
{
    m_title->SetLabel("Tool: " + name);
    UpdateProperties(name);  // 更新属性显示
}

// 从 ToolboxPanel 迁移过来的 InitToolPropertyMap 实现
void PropertyPanel::InitToolPropertyMap()
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
    ledProps.push_back(ToolProperty("Facing", "string", "West"));
    ledProps.push_back(ToolProperty("On Color", "color", "#F00000"));
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

// 从 ToolboxPanel 迁移过来的 UpdateProperties 实现
void PropertyPanel::UpdateProperties(const wxString& toolName)
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
        // 颜色类型暂时忽略
    }
}