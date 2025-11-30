#pragma once
#include <wx/wx.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/variant.h>
#include <map>
#include <vector>

// 定义属性项结构体（从 ToolboxPanel 迁移过来）
struct ToolProperty {
    wxString propName;       // 属性名
    wxString propType;       // 属性类型
    wxVariant defaultValue;  // 默认值

    ToolProperty(const wxString& name, const wxString& type, const wxVariant& value)
        : propName(name), propType(type), defaultValue(value) {
    }
};

class PropertyPanel : public wxPanel
{
public:
    PropertyPanel(wxWindow* parent);
    void ShowElement(const wxString& name, const std::map<wxString, wxVariant>& currentProps = {});    // 显示元素属性

private:
    wxStaticText* m_title;                    // 标题
    wxPropertyGrid* m_propGrid;               // 属性表格（新增）

    // 工具名 → 属性列表的映射（从 ToolboxPanel 迁移过来）
    std::map<wxString, std::vector<ToolProperty>> m_toolPropMap;

    // 私有方法
    void InitToolPropertyMap();               // 初始化属性映射表
    void UpdateProperties(const wxString& toolName, const std::map<wxString, wxVariant>& currentProps = {});  // 更新属性显示
};