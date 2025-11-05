#include "StatusBar.h"
#include "ToolManager.h"  // 为了使用 ToolType

StatusBar::StatusBar(wxWindow* parent)
    : wxStatusBar(parent, wxID_ANY)
{
    SetFieldsCount(Field_Count);
    InitializeFields();
}

void StatusBar::InitializeFields()
{
    int widths[Field_Count] = { 200, 150, 200, 100 };
    SetStatusWidths(Field_Count, widths);

    // 设置初始状态
    SetStatusText("工具: 默认工具", Field_Tool);
    SetStatusText("位置: (0, 0)", Field_MousePos);
    SetStatusText("焦点: 画布", Field_Focus);
    SetStatusText("缩放: 100%", Field_Zoom);
}

void StatusBar::UpdateToolStatus(ToolType toolType, const wxString& componentName)
{
    wxString toolName;
    switch (toolType) {
    case ToolType::DEFAULT_TOOL:
        toolName = "默认工具";
        break;
    case ToolType::SELECT_TOOL:
        toolName = "选中工具";
        break;
    case ToolType::TEXT_TOOL:
        toolName = "文本工具";
        break;
    case ToolType::COMPONENT_TOOL:
        toolName = wxString::Format("元件工具: %s", componentName);
        break;
    case ToolType::WIRE_TOOL:
        toolName = "导线工具";
        break;
    case ToolType::PAN_TOOL:
        toolName = "平移工具";
        break;
    }

    SetStatusText(wxString::Format("工具: %s", toolName), Field_Tool);
}

void StatusBar::UpdateMousePosition(const wxPoint& canvasPos)
{
    SetStatusText(wxString::Format("位置: (%d, %d)", canvasPos.x, canvasPos.y), Field_MousePos);
}

void StatusBar::UpdateFocusInfo(const wxString& focusInfo)
{
    SetStatusText(wxString::Format("焦点: %s", focusInfo), Field_Focus);
}

void StatusBar::UpdateZoomLevel(float zoom)
{
    SetStatusText(wxString::Format("缩放: %.0f%%", zoom * 100), Field_Zoom);
}

void StatusBar::SetStatusText(const wxString& text, int field)
{
    wxStatusBar::SetStatusText(text, field);
}