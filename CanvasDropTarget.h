#pragma once
#include <wx/dnd.h>
#include <wx/dataobj.h>

class CanvasPanel;

class CanvasDropTarget : public wxDropTarget
{
public:
    explicit CanvasDropTarget(CanvasPanel* canvas);

    // 以下 5 个纯虚函数必须全部实现
    wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def) override;
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) override;
    void OnLeave() override;
    bool OnDrop(wxCoord x, wxCoord y) override;
    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def) override;

private:
    CanvasPanel* m_canvas;
};