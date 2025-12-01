#include "PropertyPanel.h"

PropertyPanel::PropertyPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    // ������ֱ����
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // ��������
    m_title = new wxStaticText(this, wxID_ANY, "No element selected");
    sizer->Add(m_title, 0, wxALL, 5);

    // �������Ա���������
    m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER);
    m_propGrid->SetCaptionBackgroundColour(wxColour(230, 230, 230));

    // ��������
    wxFont font(wxFontInfo(11).FaceName("Segoe UI"));
    m_propGrid->SetFont(font);

    sizer->Add(m_propGrid, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);

    // ��ʼ������ӳ���
    InitToolPropertyMap();
}

void PropertyPanel::ShowElement(const wxString& name, const std::map<wxString, wxVariant>& currentProps)
{
    m_title->SetLabel("Tool: " + name);
    UpdateProperties(name, currentProps); // ���� currentProps �Ǻ�������������δ���������
}

// �� ToolboxPanel Ǩ�ƹ����� InitToolPropertyMap ʵ��
void PropertyPanel::InitToolPropertyMap()
{
    // -------------------------- 1. Pin (Input) ���� --------------------------
    std::vector<ToolProperty> pinInputProps;
    pinInputProps.push_back(ToolProperty("Facing", "facing", "East"));
    pinInputProps.push_back(ToolProperty("Output?", "bool", false));
    pinInputProps.push_back(ToolProperty("Data Bits", "int", 1L));
    pinInputProps.push_back(ToolProperty("Three-state?", "bool", true));
    pinInputProps.push_back(ToolProperty("Pull Behavior", "string", "Unchanged"));
    pinInputProps.push_back(ToolProperty("Label", "string", ""));
    pinInputProps.push_back(ToolProperty("Label Location", "string", "West"));
    pinInputProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    m_toolPropMap["Pin (Input)"] = pinInputProps;

    // -------------------------- 2. OR Gate ���� --------------------------
    std::vector<ToolProperty> orGateProps;
    orGateProps.push_back(ToolProperty("Facing", "facing", "East"));
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

    // -------------------------- 3. Demultiplexer ���� --------------------------
    std::vector<ToolProperty> demuxProps;
    demuxProps.push_back(ToolProperty("Facing", "facing", "East"));
    demuxProps.push_back(ToolProperty("Select Location", "string", "Bottom/Left"));
    demuxProps.push_back(ToolProperty("Select Bits", "int", 1L));
    demuxProps.push_back(ToolProperty("Data Bits", "int", 1L));
    demuxProps.push_back(ToolProperty("Three-state?", "bool", false));
    demuxProps.push_back(ToolProperty("Disabled Output", "string", "Floating"));
    demuxProps.push_back(ToolProperty("Include Enable?", "bool", true));
    m_toolPropMap["Demultiplexer"] = demuxProps;

    // -------------------------- 4. Comparator ���� --------------------------
    std::vector<ToolProperty> comparatorProps;
    comparatorProps.push_back(ToolProperty("Data Bits", "int", 8L));
    comparatorProps.push_back(ToolProperty("Numeric Type", "string", "2's Complement"));
    m_toolPropMap["Comparator"] = comparatorProps;

    // -------------------------- 5. S-R Flip-Flop ���� --------------------------
    std::vector<ToolProperty> srFlipFlopProps;
    srFlipFlopProps.push_back(ToolProperty("Trigger", "facing", "Rising Edge"));
    srFlipFlopProps.push_back(ToolProperty("Label", "string", ""));
    srFlipFlopProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    m_toolPropMap["SR Flip-Flop"] = srFlipFlopProps;

    // -------------------------- 6. LED ���� --------------------------
    std::vector<ToolProperty> ledProps;
    ledProps.push_back(ToolProperty("Facing", "facing", "West"));
    ledProps.push_back(ToolProperty("On Color", "color", "#F00000"));
    ledProps.push_back(ToolProperty("Off Color", "color", "#404040"));
    ledProps.push_back(ToolProperty("Active On High?", "bool", true));
    ledProps.push_back(ToolProperty("Label", "string", ""));
    ledProps.push_back(ToolProperty("Label Location", "string", "Center"));
    ledProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    ledProps.push_back(ToolProperty("Label Color", "color", "#000000"));
    m_toolPropMap["LED"] = ledProps;

    // -------------------------- ������������ --------------------------
    m_toolPropMap["Wire"] = std::vector<ToolProperty>();

    std::vector<ToolProperty> buttonProps;
    buttonProps.push_back(ToolProperty("Facing", "facing", "North"));
    buttonProps.push_back(ToolProperty("Trigger", "string", "Edge"));
    buttonProps.push_back(ToolProperty("Label", "string", ""));
    buttonProps.push_back(ToolProperty("Label Location", "string", "South"));
    m_toolPropMap["Button"] = buttonProps;
}

// �� ToolboxPanel Ǩ�ƹ����� UpdateProperties ʵ��
void PropertyPanel::UpdateProperties(const wxString& toolName, const std::map<wxString, wxVariant>& currentProps)
{
    // 1. ���ԭ������
    m_propGrid->Clear();

    // 2. ���ӷ������
    wxPropertyCategory* category = new wxPropertyCategory(wxString::Format("Tool: %s", toolName));
    m_propGrid->Append(category);

    // 3. ��ӳ����л�ȡ��ǰ���ߵ������б�
    auto it = m_toolPropMap.find(toolName);
    if (it == m_toolPropMap.end()) {
        wxStringProperty* noProps = new wxStringProperty("Info", "Info", "No configurable properties");
        m_propGrid->Append(noProps);
        return;
    }

    // 4. �������ԣ�ֻ֧�ֻ������ͣ�
    std::vector<ToolProperty> props = it->second;
    for (auto& prop : props) {
        if (prop.propType == "facing") {
            // ��������ѡ�East/South/West/North��
            wxArrayString options;
            options.Add("East");
            options.Add("South");
            options.Add("West");
            options.Add("North");
            auto facingProp = new wxEnumProperty(prop.propName, prop.propName, options);

            // �� currentProps ��ȡ��ǰֵ������У���û�о���Ĭ��ֵ
            auto currIt = currentProps.find(prop.propName);
            if (currIt != currentProps.end()) {
                facingProp->SetValue(currIt->second);
            }
            else {
                facingProp->SetValue(prop.defaultValue);
            }
            m_propGrid->Append(facingProp);
        }
        else if (prop.propType == "bool") {
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
        // ��ɫ������ʱ����
    }
}