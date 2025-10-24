#pragma once
#include <wx/wx.h>
#include <vector>
#include <variant>

struct Point { int x, y; };
struct Line { Point start, end; wxColour color; };
struct PolyShape { std::vector<Point> pts; wxColour color; };
struct Circle { Point center; int radius; wxColour color; };
struct Text { Point pos; wxString text; int fontSize; wxColour color; };
struct Pin{
    Point     pos;
    wxString  name;
    bool      isInput;   // true = 输入引脚，false = 输出引脚
};

using Shape = std::variant<Line, PolyShape, Circle, Text>;

class CanvasElement
{
public:
    CanvasElement(const wxString& name, const wxPoint& pos);
    void Draw(wxDC& dc) const;
    void AddShape(const Shape& shape) { m_shapes.push_back(shape); }
    void AddInputPin(const Point& p, const wxString& name) { m_inputPins.push_back({ p, name, true }); }
    void AddOutputPin(const Point& p, const wxString& name) { m_outputPins.push_back({ p, name, false }); }
    void SetPos(const wxPoint& p) { m_pos = p; }

    const wxString& GetName() const { return m_name; }
    const wxPoint& GetPos() const { return m_pos; }
    const std::vector<Shape>& GetShapes() const { return m_shapes; }
    const std::vector<struct Pin>& GetInputPins() const { return m_inputPins; }
    const std::vector<struct Pin>& GetOutputPins() const { return m_outputPins; }

    wxRect GetBounds() const; // 添加边界框计算方法

private:
    wxString m_name;
    wxPoint  m_pos;
    std::vector<Shape> m_shapes;
    std::vector<struct Pin> m_inputPins;
    std::vector<struct Pin> m_outputPins;
};