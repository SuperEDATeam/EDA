#pragma once
#include <wx/wx.h>

class CanvasPanel : public wxPanel
{
public:
    CanvasPanel(wxWindow* parent);
private:
    void OnPaint(wxPaintEvent&);
    wxDECLARE_EVENT_TABLE();
};