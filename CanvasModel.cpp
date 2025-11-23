#include "CanvasModel.h"
#include "CanvasElement.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <fstream>
#include <json/json.h>
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

        // 设置元件ID
        ce.SetId(id);

        // ������
        for (const auto& pin : elem["inputPins"])
            ce.AddInputPin({ pin["x"].asInt(), pin["y"].asInt() }, wxString::FromUTF8(pin["name"].asString()));
        for (const auto& pin : elem["outputPins"])
            ce.AddOutputPin({ pin["x"].asInt(), pin["y"].asInt() }, wxString::FromUTF8(pin["name"].asString()));

        // ��ͼ��
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
                int startX = shape["start"]["x"].asInt();
                int startY = shape["start"]["y"].asInt();
                int endX = shape["end"]["x"].asInt();
                int endY = shape["end"]["y"].asInt();
                // �����־��ȷ��ˮƽֱ�ߣ�y������ͬ��������
                MyLog("Loaded Line: start(%d,%d) �� end(%d,%d) [type: %s]\n",
                    startX, startY, endX, endY, type.ToUTF8().data());
                ce.AddShape(Line{
                    {startX, startY},
                    {endX, endY},
                    color
                    });
            }
            else if (type == "circle") {

                bool fill = shape.get("fill", false).asBool();
                wxColour fillColor = fill ? wxColour(shape.get("fillColor", "#808080").asString()) : wxColour(0, 0, 0);

                ce.AddShape(Circle{
                    {shape["x"].asInt(), shape["y"].asInt()},
                    shape["r"].asInt(),
                    color,
                    fill,
                    fillColor
                    });
            }
            else if (type == "text") {
                wxString text = wxString::FromUTF8(shape["text"].asString());
                //text.Replace("&", "&&");
                ce.AddShape(Text{
                    {shape["x"].asInt(), shape["y"].asInt()},
                    text,

                    shape["fontSize"].asInt(),
                    color
                    });
            }
            else if (type == "path") {
                ce.AddShape(Path{
                    shape["d"].asString(),
                    wxColour(shape["stroke"].asString()),
                    shape["strokeWidth"].asInt(),
                    shape["fill"].asString() != "none"
                    });
            }
            else if (type == "ArcShape") {
                ce.AddShape(ArcShape{
                    {shape["center"]["x"].asInt(), shape["center"]["y"].asInt()},
                    shape["radius"].asInt(),
                    shape["startAngle"].asDouble(),
                    shape["endAngle"].asDouble(),
                    color
                    });
            }

            // 椭半圆的绘制
            else if (type == "BezierShape") {
                Point p0 = { shape["p0"]["x"].asInt(), shape["p0"]["y"].asInt() };
                Point p1 = { shape["p1"]["x"].asInt(), shape["p1"]["y"].asInt() };
                Point p2 = { shape["p2"]["x"].asInt(), shape["p2"]["y"].asInt() };

                ce.AddShape(BezierShape{
                    p0, p1, p2,
                    color
                    });
            }

        }
        out.push_back(ce);
    }
    return out;
}