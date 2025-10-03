#include "CanvasDropTarget.h"
#include "CanvasPanel.h"
#include "CanvasModel.h"
#include <json/json.h>
#include "my_log.h"

CanvasDropTarget::CanvasDropTarget(CanvasPanel* canvas)
    : m_canvas(canvas)
{
    SetDataObject(new wxTextDataObject());
}

wxDragResult CanvasDropTarget::OnEnter(wxCoord, wxCoord, wxDragResult def)
{
    return def;
}
wxDragResult CanvasDropTarget::OnDragOver(wxCoord, wxCoord, wxDragResult def)
{
    return def;
}
void CanvasDropTarget::OnLeave() {}

bool CanvasDropTarget::OnDrop(wxCoord x, wxCoord y)
{
    return OnData(x, y, wxDragCopy);
}

wxDragResult CanvasDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult)
{
    MyLog(">>> OnData: entry (%d,%d)\n", (int)x, (int)y);

    static const wxDataFormat myFormat("application/my-canvas-elem");
    //wxCustomDataObject data(myFormat);
    //if (!GetDataObject()) {
    //    MyLog(">>> OnData: no data object\n");
    //    return wxDragNone;
    //}
    //// 直接取字节
    //std::string text((const char*)data.GetData(), data.GetSize());
    //MyLog(">>> OnData: text=[%s]\n", text.c_str());

    wxCustomDataObject data(myFormat);

    if (!GetDataObject()) {
        MyLog(">>> OnData: no data object\n");
        return wxDragNone;
    }
    std::string text((const char*)data.GetData(), data.GetSize());
    MyLog(">>> OnData: received %zu bytes\n", text.size());

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(text, root)) {
        MyLog(">>> OnData: JSON parse error\n");
        return wxDragNone;
    }
    if (root["action"].asString() != "add") {
        MyLog(">>> OnData: action != add\n");
        return wxDragNone;
    }
    wxString type = wxString::FromUTF8(root["type"].asString());
    MyLog(">>> OnData: type=[%s]\n", type.ToUTF8().data());

    extern std::vector<CanvasElement> g_elements;
    auto it = std::find_if(g_elements.begin(), g_elements.end(),
        [&](const CanvasElement& e)
        { return e.GetName() == type; });
    if (it == g_elements.end()) {
        MyLog(">>> OnData: template not found\n");
        return wxDragNone;
    }

    CanvasElement clone = *it;
    clone.SetPos(wxPoint(x, y));
    m_canvas->AddElement(clone);
    MyLog(">>> OnData: element added\n");
    return wxDragCopy;
}