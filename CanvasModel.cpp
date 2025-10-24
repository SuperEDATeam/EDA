#include "CanvasModel.h"
#include "CanvasElement.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <fstream>
#include "3rd/json/json.h"
#include "my_log.h"

std::vector<CanvasElement> g_elements;

std::vector<CanvasElement> LoadCanvasElements(const wxString& jsonPath)
{
    /*std::ifstream f(jsonPath.ToStdString(), std::ios::binary);
    if (!f) return {};*/

    wxString absPath = wxFileName(jsonPath).GetFullPath();
    MyLog("LoadCanvas: absolute path [%s]\n", absPath.ToUTF8().data());
    MyLog("LoadCanvas: file exists = %d\n", wxFileName::FileExists(jsonPath));
    
    std::ifstream test(jsonPath.ToStdString());
    MyLog("LoadCanvas: ifstream good = %d\n", test.good());

    // ② 看文件大小
    if (test.good()) {
        test.seekg(0, std::ios::end);
        size_t size = test.tellg();
        MyLog("LoadCanvas: file size = %zu bytes\n", size);
        test.seekg(0, std::ios::beg);
    }

    std::ifstream f(jsonPath.ToStdString(), std::ios::binary);
    MyLog("LoadCanvas: try open [%s]\n", jsonPath.ToUTF8().data());
    if (!f) { MyLog("LoadCanvas: file not found!\n"); return {}; }

    

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, f, &root, &errs)) return {};

    std::vector<CanvasElement> out;
    for (const auto& elem : root) {
        wxString id = wxString::FromUTF8(elem["id"].asString());
        wxString name = wxString::FromUTF8(elem["name"].asString());
        wxPoint  pos(elem["anchorPoint"][0].asInt(), elem["anchorPoint"][1].asInt());
        CanvasElement ce(name, pos);

        // 读引脚
        for (const auto& pin : elem["inputPins"])
            ce.AddInputPin({ pin["x"].asInt(), pin["y"].asInt() }, wxString::FromUTF8(pin["name"].asString()));
        for (const auto& pin : elem["outputPins"])
            ce.AddOutputPin({ pin["x"].asInt(), pin["y"].asInt() }, wxString::FromUTF8(pin["name"].asString()));

        // 读图形
        for (const auto& shape : elem["shapes"]) {
            wxColour color(shape["color"].asString());
            wxString type = shape["type"].asString();

            if (type == "polygon") {
                std::vector<Point> pts;
                for (const auto& pt : shape["points"])
                    pts.push_back({ pt[0].asInt(), pt[1].asInt() });
                ce.AddShape(PolyShape{ pts, color });
            }
            else if (type == "line") {
                ce.AddShape(Line{
                    {shape["x1"].asInt(), shape["y1"].asInt()},
                    {shape["x2"].asInt(), shape["y2"].asInt()},
                    color
                    });
            }
            else if (type == "circle") {
                ce.AddShape(Circle{
                    {shape["x"].asInt(), shape["y"].asInt()},
                    shape["r"].asInt(),
                    color
                    });
            }
            else if (type == "text") {
                ce.AddShape(Text{
                    {shape["x"].asInt(), shape["y"].asInt()},
                    wxString::FromUTF8(shape["text"].asString()),
                    shape["fontSize"].asInt(),
                    color
                    });
            }
        }
        out.push_back(ce);
    }
    return out;
}