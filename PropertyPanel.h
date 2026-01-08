#pragma once
#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/variant.h>
#include <map>
#include <vector>

class CanvasElement;

struct ToolProperty {
    wxString propName;
    wxString propType;
    wxVariant defaultValue;

    ToolProperty(const wxString& name, const wxString& type, const wxVariant& value)
        : propName(name), propType(type), defaultValue(value) {
    }
};

class PropertyPanel : public wxPanel
{
public:
    PropertyPanel(wxWindow* parent);
    void ShowElement(const wxString& name, const std::map<wxString, wxVariant>& currentProps = {});
    
    // 显示选中的画布元件属性
    void ShowCanvasElement(CanvasElement* element);
    
    // 显示通用逻辑门属性
    void ShowGateProperties(CanvasElement* element);
    
    // 获取当前编辑的元件
    CanvasElement* GetCurrentElement() const { return m_currentElement; }

private:
    wxStaticText* m_title;
    wxPropertyGrid* m_propGrid;
    CanvasElement* m_currentElement = nullptr;
    
    // 动态Negate属性列表
    std::vector<wxBoolProperty*> m_negateProps;

    std::map<wxString, std::vector<ToolProperty>> m_toolPropMap;

    void InitToolPropertyMap();
    void UpdateProperties(const wxString& toolName, const std::map<wxString, wxVariant>& currentProps = {});
    
    // AND门属性编辑
    void ShowAndGateProperties(CanvasElement* element);
    void OnPropertyChanged(wxPropertyGridEvent& event);
    
    // 更新Negate选项数量
    void UpdateNegateOptions(int count, CanvasElement* element);
    
    wxDECLARE_EVENT_TABLE();
};
