#include "PropertyPanel.h"
#include "CanvasElement.h"

wxBEGIN_EVENT_TABLE(PropertyPanel, wxPanel)
    EVT_PG_CHANGED(wxID_ANY, PropertyPanel::OnPropertyChanged)
wxEND_EVENT_TABLE()

PropertyPanel::PropertyPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_title = new wxStaticText(this, wxID_ANY, "No element selected");
    sizer->Add(m_title, 0, wxALL, 5);

    m_propGrid = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER);
    m_propGrid->SetCaptionBackgroundColour(wxColour(230, 230, 230));

    wxFont font(wxFontInfo(11).FaceName("Segoe UI"));
    m_propGrid->SetFont(font);

    sizer->Add(m_propGrid, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);

    InitToolPropertyMap();
}

void PropertyPanel::ShowElement(const wxString& name, const std::map<wxString, wxVariant>& currentProps)
{
    m_title->SetLabel("Tool: " + name);
    UpdateProperties(name, currentProps);
}

void PropertyPanel::InitToolPropertyMap()
{
    // AND Gate 属性
    std::vector<ToolProperty> andGateProps;
    andGateProps.push_back(ToolProperty("Facing", "facing", "East"));
    andGateProps.push_back(ToolProperty("Data Bits", "int", 1L));
    andGateProps.push_back(ToolProperty("Gate Size", "string", "Medium"));
    andGateProps.push_back(ToolProperty("Number Of Inputs", "int", 2L));
    andGateProps.push_back(ToolProperty("Output Value", "string", "0/1"));
    andGateProps.push_back(ToolProperty("Label", "string", ""));
    andGateProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    andGateProps.push_back(ToolProperty("Negate 1 (Top)", "bool", false));
    andGateProps.push_back(ToolProperty("Negate 2 (Bottom)", "bool", false));
    m_toolPropMap["AND Gate"] = andGateProps;
    m_toolPropMap["AND Gate (Rect)"] = andGateProps;

    // Pin (Input) 属性
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

    // OR Gate 属性
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

    // Demultiplexer 属性
    std::vector<ToolProperty> demuxProps;
    demuxProps.push_back(ToolProperty("Facing", "facing", "East"));
    demuxProps.push_back(ToolProperty("Select Location", "string", "Bottom/Left"));
    demuxProps.push_back(ToolProperty("Select Bits", "int", 1L));
    demuxProps.push_back(ToolProperty("Data Bits", "int", 1L));
    demuxProps.push_back(ToolProperty("Three-state?", "bool", false));
    demuxProps.push_back(ToolProperty("Disabled Output", "string", "Floating"));
    demuxProps.push_back(ToolProperty("Include Enable?", "bool", true));
    m_toolPropMap["Demultiplexer"] = demuxProps;

    // Comparator 属性
    std::vector<ToolProperty> comparatorProps;
    comparatorProps.push_back(ToolProperty("Data Bits", "int", 8L));
    comparatorProps.push_back(ToolProperty("Numeric Type", "string", "2's Complement"));
    m_toolPropMap["Comparator"] = comparatorProps;

    // SR Flip-Flop 属性
    std::vector<ToolProperty> srFlipFlopProps;
    srFlipFlopProps.push_back(ToolProperty("Trigger", "facing", "Rising Edge"));
    srFlipFlopProps.push_back(ToolProperty("Label", "string", ""));
    srFlipFlopProps.push_back(ToolProperty("Label Font", "string", "SansSerif Plain 12"));
    m_toolPropMap["SR Flip-Flop"] = srFlipFlopProps;

    // LED 属性
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

    m_toolPropMap["Wire"] = std::vector<ToolProperty>();

    std::vector<ToolProperty> buttonProps;
    buttonProps.push_back(ToolProperty("Facing", "facing", "North"));
    buttonProps.push_back(ToolProperty("Trigger", "string", "Edge"));
    buttonProps.push_back(ToolProperty("Label", "string", ""));
    buttonProps.push_back(ToolProperty("Label Location", "string", "South"));
    m_toolPropMap["Button"] = buttonProps;
}


void PropertyPanel::UpdateProperties(const wxString& toolName, const std::map<wxString, wxVariant>& currentProps)
{
    m_propGrid->Clear();

    wxPropertyCategory* category = new wxPropertyCategory(wxString::Format("Tool: %s", toolName));
    m_propGrid->Append(category);

    auto it = m_toolPropMap.find(toolName);
    if (it == m_toolPropMap.end()) {
        wxStringProperty* noProps = new wxStringProperty("Info", "Info", "No configurable properties");
        m_propGrid->Append(noProps);
        return;
    }

    std::vector<ToolProperty> props = it->second;
    for (auto& prop : props) {
        wxVariant defaultValue = prop.defaultValue;

        auto currIt = currentProps.find(prop.propName);
        if (currIt != currentProps.end()) {
            defaultValue = currIt->second;
        }

        if (prop.propType == "facing") {
            wxArrayString options;
            options.Add("East");
            options.Add("South");
            options.Add("West");
            options.Add("North");
            auto facingProp = new wxEnumProperty(prop.propName, prop.propName, options);
            facingProp->SetValue(defaultValue);
            m_propGrid->Append(facingProp);
        }
        else if (prop.propType == "bool") {
            wxBoolProperty* boolProp = new wxBoolProperty(prop.propName, prop.propName);
            boolProp->SetValue(defaultValue.GetBool());
            m_propGrid->Append(boolProp);
        }
        else if (prop.propType == "int") {
            wxIntProperty* intProp = new wxIntProperty(prop.propName, prop.propName);
            intProp->SetValue(defaultValue.GetLong());
            m_propGrid->Append(intProp);
        }
        else if (prop.propType == "string") {
            wxStringProperty* strProp = new wxStringProperty(prop.propName, prop.propName);
            strProp->SetValue(defaultValue.GetString());
            m_propGrid->Append(strProp);
        }
    }

    m_propGrid->Refresh();
}

void PropertyPanel::ShowCanvasElement(CanvasElement* element)
{
    m_currentElement = element;
    
    if (!element) {
        m_title->SetLabel("No element selected");
        m_propGrid->Clear();
        return;
    }
    
    // 只有 AND 门显示属性编辑面板
    if (element->IsAndGate()) {
        ShowGateProperties(element);
    } else {
        ShowElement(element->GetName());
    }
}

void PropertyPanel::ShowAndGateProperties(CanvasElement* element)
{
    if (!element) return;
    
    m_propGrid->Clear();
    m_title->SetLabel("AND Gate Properties");
    
    const AndGateProperties& props = element->GetAndGateProps();
    
    wxPropertyCategory* category = new wxPropertyCategory("AND Gate");
    m_propGrid->Append(category);
    
    // Facing
    wxArrayString facingOptions;
    facingOptions.Add("East");
    facingOptions.Add("West");
    facingOptions.Add("North");
    facingOptions.Add("South");
    wxEnumProperty* facingProp = new wxEnumProperty("Facing", "Facing", facingOptions);
    int facingIdx = facingOptions.Index(props.facing);
    if (facingIdx != wxNOT_FOUND) facingProp->SetValue(facingIdx);
    m_propGrid->Append(facingProp);
    
    // Data Bits
    wxIntProperty* dataBitsProp = new wxIntProperty("Data Bits", "DataBits", props.dataBits);
    dataBitsProp->SetAttribute(wxPG_ATTR_MIN, 1);
    dataBitsProp->SetAttribute(wxPG_ATTR_MAX, 32);
    m_propGrid->Append(dataBitsProp);
    
    // Gate Size
    wxArrayString sizeOptions;
    sizeOptions.Add("Narrow");
    sizeOptions.Add("Medium");
    sizeOptions.Add("Wide");
    wxEnumProperty* sizeProp = new wxEnumProperty("Gate Size", "GateSize", sizeOptions);
    int sizeIdx = sizeOptions.Index(props.gateSize);
    if (sizeIdx != wxNOT_FOUND) sizeProp->SetValue(sizeIdx);
    m_propGrid->Append(sizeProp);
    
    // Number of Inputs
    wxIntProperty* inputsProp = new wxIntProperty("Number of Inputs", "NumberOfInputs", props.numberOfInputs);
    inputsProp->SetAttribute(wxPG_ATTR_MIN, 2);
    inputsProp->SetAttribute(wxPG_ATTR_MAX, 32);
    m_propGrid->Append(inputsProp);
    
    // Label
    wxStringProperty* labelProp = new wxStringProperty("Label", "Label", props.label);
    m_propGrid->Append(labelProp);
    
    // Label Font
    wxFont labelFont = props.GetLabelFontAsWxFont();
    wxFontProperty* fontProp = new wxFontProperty("Label Font", "LabelFont", labelFont);
    m_propGrid->Append(fontProp);
    
    // Negate inputs
    wxPropertyCategory* negateCategory = new wxPropertyCategory("Negate Inputs");
    m_propGrid->Append(negateCategory);
    
    for (int i = 0; i < props.numberOfInputs && i < 5; i++) {
        wxString name;
        if (i == 0) name = "Negate 1 (Top)";
        else if (i == props.numberOfInputs - 1) name = wxString::Format("Negate %d (Bottom)", i + 1);
        else name = wxString::Format("Negate %d", i + 1);
        
        bool negateValue = (i < (int)props.negateInputs.size()) ? props.negateInputs[i] : false;
        wxBoolProperty* negateProp = new wxBoolProperty(name, wxString::Format("Negate%d", i), negateValue);
        negateProp->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
        m_propGrid->Append(negateProp);
    }
    
    m_propGrid->Refresh();
}

void PropertyPanel::OnPropertyChanged(wxPropertyGridEvent& event)
{
    if (!m_currentElement) return;
    
    wxPGProperty* prop = event.GetProperty();
    if (!prop) return;
    
    wxString propName = prop->GetName();
    
    // 只处理 AND 门的属性修改
    if (m_currentElement->IsAndGate()) {
        GateProperties& gateProps = m_currentElement->GetGateProps();
        
        if (propName == "Facing") {
            wxArrayString options;
            options.Add("East");
            options.Add("West");
            options.Add("North");
            options.Add("South");
            int idx = prop->GetValue().GetLong();
            if (idx >= 0 && idx < (int)options.size()) {
                gateProps.facing = options[idx];
            }
        }
        else if (propName == "DataBits") {
            gateProps.dataBits = prop->GetValue().GetLong();
        }
        else if (propName == "GateSize") {
            wxArrayString options;
            options.Add("Narrow");
            options.Add("Medium");
            options.Add("Wide");
            int idx = prop->GetValue().GetLong();
            if (idx >= 0 && idx < (int)options.size()) {
                gateProps.gateSize = options[idx];
            }
        }
        else if (propName == "NumberOfInputs") {
            int newInputs = prop->GetValue().GetLong();
            if (newInputs != gateProps.numberOfInputs) {
                gateProps.numberOfInputs = newInputs;
                gateProps.Validate();
                
                // 重新生成 AND 门形状
                m_currentElement->RegenerateShapes();
                
                // 刷新属性面板以更新 Negate 选项
                CallAfter([this]() {
                    if (m_currentElement) {
                        ShowGateProperties(m_currentElement);
                    }
                });
            }
        }
        else if (propName == "Label") {
            gateProps.label = prop->GetValue().GetString();
        }
        else if (propName == "LabelFont") {
            wxFont font;
            font << prop->GetValue();
            gateProps.SetLabelFontFromWxFont(font);
        }
        else if (propName.StartsWith("Negate")) {
            wxString idxStr = propName.Mid(6);
            long idx = 0;
            if (idxStr.ToLong(&idx) && idx >= 0 && idx < (int)gateProps.negateInputs.size()) {
                gateProps.negateInputs[idx] = prop->GetValue().GetBool();
            }
        }
        
        // 重新生成 AND 门形状
        m_currentElement->RegenerateShapes();
        
        // 同步到 AndGateProps
        AndGateProperties& andProps = m_currentElement->GetAndGateProps();
        andProps = gateProps;
    }
    
    wxWindow* topWindow = wxTheApp->GetTopWindow();
    if (topWindow) {
        wxWindowList& children = topWindow->GetChildren();
        for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
            (*it)->Refresh();
        }
        topWindow->Refresh();
        topWindow->Update();
    }
}

void PropertyPanel::ShowGateProperties(CanvasElement* element)
{
    // 只处理 AND 门
    if (!element || !element->IsAndGate()) return;
    
    m_currentElement = element;
    m_propGrid->Clear();
    m_negateProps.clear();
    
    wxString gateType = element->GetId();
    m_title->SetLabel(gateType + " Properties");
    
    GateProperties& props = element->GetGateProps();
    props.Validate();
    
    wxPropertyCategory* category = new wxPropertyCategory("AND Gate Properties");
    m_propGrid->Append(category);
    
    // Facing
    wxArrayString facingOptions;
    facingOptions.Add("East");
    facingOptions.Add("West");
    facingOptions.Add("North");
    facingOptions.Add("South");
    wxEnumProperty* facingProp = new wxEnumProperty("Facing", "Facing", facingOptions);
    int facingIdx = facingOptions.Index(props.facing);
    if (facingIdx != wxNOT_FOUND) facingProp->SetValue(facingIdx);
    m_propGrid->Append(facingProp);
    
    // Data Bits
    wxIntProperty* dataBitsProp = new wxIntProperty("Data Bits", "DataBits", props.dataBits);
    dataBitsProp->SetAttribute(wxPG_ATTR_MIN, 1);
    dataBitsProp->SetAttribute(wxPG_ATTR_MAX, 32);
    m_propGrid->Append(dataBitsProp);
    
    // Gate Size
    wxArrayString sizeOptions;
    sizeOptions.Add("Narrow");
    sizeOptions.Add("Medium");
    sizeOptions.Add("Wide");
    wxEnumProperty* sizeProp = new wxEnumProperty("Gate Size", "GateSize", sizeOptions);
    int sizeIdx = sizeOptions.Index(props.gateSize);
    if (sizeIdx != wxNOT_FOUND) sizeProp->SetValue(sizeIdx);
    m_propGrid->Append(sizeProp);
    
    // Number of Inputs
    wxIntProperty* inputsProp = new wxIntProperty("Number of Inputs", "NumberOfInputs", props.numberOfInputs);
    inputsProp->SetAttribute(wxPG_ATTR_MIN, 2);
    inputsProp->SetAttribute(wxPG_ATTR_MAX, 32);
    m_propGrid->Append(inputsProp);
    
    // Label
    wxStringProperty* labelProp = new wxStringProperty("Label", "Label", props.label);
    m_propGrid->Append(labelProp);
    
    // Label Font
    wxFont labelFont = props.GetLabelFontAsWxFont();
    wxFontProperty* fontProp = new wxFontProperty("Label Font", "LabelFont", labelFont);
    m_propGrid->Append(fontProp);
    
    // Negate inputs
    UpdateNegateOptions(props.numberOfInputs, element);
    
    m_propGrid->Refresh();
}

void PropertyPanel::UpdateNegateOptions(int count, CanvasElement* element)
{
    if (!element) return;
    
    GateProperties& props = element->GetGateProps();
    
    wxPropertyCategory* negateCategory = new wxPropertyCategory("Negate Inputs");
    m_propGrid->Append(negateCategory);
    
    m_negateProps.clear();
    
    for (int i = 0; i < count && i < 32; i++) {
        wxString name;
        if (i == 0) {
            name = "Negate 1 (Top)";
        } else if (i == count - 1) {
            name = wxString::Format("Negate %d (Bottom)", i + 1);
        } else {
            name = wxString::Format("Negate %d", i + 1);
        }
        
        bool negateValue = (i < (int)props.negateInputs.size()) ? props.negateInputs[i] : false;
        wxBoolProperty* negateProp = new wxBoolProperty(name, wxString::Format("Negate%d", i), negateValue);
        negateProp->SetAttribute(wxPG_BOOL_USE_CHECKBOX, true);
        m_propGrid->Append(negateProp);
        m_negateProps.push_back(negateProp);
    }
}
