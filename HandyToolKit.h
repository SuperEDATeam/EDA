#pragma once
// HandyToolKit.h
#include <wx/wx.h>
#include <wx/popupwin.h>
#include "ToolStateMachine.h"
#include "CanvasPanel.h"

class ToolStateMachine;

class HandyToolKit : public wxPopupWindow
{
public:
    HandyToolKit(CanvasPanel* parent, ToolStateMachine* toolstate);
    ~HandyToolKit() = default;

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

    struct ToolInfo {
        wxString name;
        wxBitmap icon;
        wxRect rect;
        std::function<void()> action;
    };

    std::vector<ToolInfo> m_tools;
    int m_selectedTool;
    int m_hoveredTool;
	ToolStateMachine* m_toolStateMachine;

	CanvasPanel* m_canvas;

    wxDECLARE_EVENT_TABLE();
};