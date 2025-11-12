#pragma once
// QuickToolbar.h
#include <wx/wx.h>
#include <wx/popupwin.h>
#include "MainFrame.h"

class MainFrame;

class QuickToolBar : public wxPopupWindow
{
public:
    QuickToolBar(wxWindow* parent);
    ~QuickToolBar() = default;

    void CreateTools();
    int GetSelectedTool() const { return m_selectedTool; }

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnRightUp(wxMouseEvent& event);
    void OnKillFocus(wxFocusEvent& event);

    void DrawSemiTransparentOverlay(wxDC& dc, const wxRect& rect, const wxColour& color, int alpha);
    void DrawGlowBorder(wxDC& dc, const wxRect& rect, const wxColour& color, int glowWidth);
    void DrawGradientHighlight(wxDC& dc, const wxRect& rect, const wxColour& startColor, const wxColour& endColor);
    void DrawToolButton(wxDC& dc, const wxRect& rect, bool isHovered);

    MainFrame* GetMainFrame(wxWindow* window);

    struct ToolInfo {
        wxString name;
        wxBitmap icon;
        wxRect rect;
        std::function<void()> action;
    };

    std::vector<ToolInfo> m_tools;
    int m_selectedTool;
    int m_hoveredTool;

    wxDECLARE_EVENT_TABLE();
};