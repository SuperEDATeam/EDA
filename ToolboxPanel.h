#pragma once
#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/propgrid/propgrid.h> // 包含属性表格头文件
#include <wx/propgrid/advprops.h> 
#include <wx/variant.h> 
#include <map>
#include <vector>

// 定义属性项结构体（存储单个属性的信息）
struct ToolProperty {
    wxString propName;       // 属性名（如"On Color"）
    wxString propType;       // 属性类型（"bool""int""string""color"）
    wxVariant defaultValue;  // 默认值（支持不同类型）

    // 添加构造函数以解决 vector 初始化问题
    ToolProperty(const wxString& name, const wxString& type, const wxVariant& value)
        : propName(name), propType(type), defaultValue(value) {
    }
};

class ToolboxPanel : public wxPanel
{
public:
    explicit ToolboxPanel(wxWindow* parent);
    void Rebuild();                 // 外部调用，重建工具树
private:
    wxTreeCtrl* m_tree;
    wxImageList* m_imgList;         // 图像列表
    wxPropertyGrid* m_propGrid;     // 新增：属性表格组件
    // 新增：工具名 → 属性列表的映射（存储每个工具的所有属性）
    std::map<wxString, std::vector<ToolProperty>> m_toolPropMap;

    // 原有函数声明
    void LoadToolIcon(const wxString& toolName, const wxString& pngFileName);
    int GetToolIconIndex(const wxString& toolName);
    void OnItemActivated(wxTreeEvent& evt);
    void OnBeginDrag(wxTreeEvent& evt);

    // 新增：事件处理函数
    void OnToolSelected(wxTreeEvent& evt); // 工具选中事件（核心）
    void InitToolPropertyMap();            // 初始化工具-属性映射表
    void UpdatePropertyPanel(const wxString& toolName); // 更新属性面板

    wxDECLARE_EVENT_TABLE();
};