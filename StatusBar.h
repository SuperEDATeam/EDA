#pragma once
#include <wx/wx.h>
#include <wx/statusbr.h>

// 前向声明
enum class ToolType;

class StatusBar : public wxStatusBar
{
public:
    StatusBar(wxWindow* parent);

    // 更新状态栏显示
    void UpdateToolStatus(ToolType toolType, const wxString& componentName = "");
    void UpdateMousePosition(const wxPoint& canvasPos);
    void UpdateFocusInfo(const wxString& focusInfo);
    void UpdateZoomLevel(float zoom);

    // 通用状态更新
    void SetStatusText(const wxString& text, int field = 0);

private:
    enum StatusField {
        Field_Tool = 0,
        Field_MousePos,
        Field_Focus,
        Field_Zoom,
        Field_Count
    };

    void InitializeFields();
};