#pragma once

#include <wx/wx.h>
#include <wx/dcgraph.h>  
#include <wx/graphics.h>  
#include <vector>
#include <variant>

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Line {
    Point start, end;
    wxColour color;
    Line(Point s = Point(), Point e = Point(), wxColour c = wxColour(0, 0, 0))
        : start(s), end(e), color(c) {
    }
};

struct PolyShape {
    std::vector<Point> pts;
    wxColour color;
    PolyShape(std::vector<Point> p = {}, wxColour c = wxColour(0, 0, 0))
        : pts(p), color(c) {
    }
};

struct Circle {
    Point center;
    int radius;
    wxColour color;

    bool fill;          // 新增：是否填充
    wxColour fillColor; // 新增：填充颜色
    Circle(Point c = Point(), int r = 0, wxColour col = wxColour(0, 0, 0),
        bool f = false, wxColour fc = wxColour(128, 128, 128))
        : center(c), radius(r), color(col), fill(f), fillColor(fc) {

    }
};

struct Text {
    Point pos;
    wxString text;
    int fontSize;
    wxColour color;
    Text(Point p = Point(), wxString t = "", int fs = 10, wxColour c = wxColour(0, 0, 0))
        : pos(p), text(t), fontSize(fs), color(c) {
    }
};

struct Pin {
    Point pos;
    wxString name;
    bool isInput;
    Pin(Point p = Point(), wxString n = "", bool input = true)
        : pos(p), name(n), isInput(input) {
    }
};

// 重命名 Arc 为 ArcShape 避免可能的命名冲突
struct ArcShape {
    Point center;
    int radius;
    double startAngle;
    double endAngle;
    wxColour color;
    ArcShape(Point c = Point(), int r = 0, double sa = 0, double ea = 0, wxColour col = wxColour(0, 0, 0))
        : center(c), radius(r), startAngle(sa), endAngle(ea), color(col) {
    }
};

struct BezierShape {
    Point p0, p1, p2;
    wxColour color;
    BezierShape(Point p0 = Point(), Point p1 = Point(), Point p2 = Point(), wxColour c = wxColour(0, 0, 0))
        : p0(p0), p1(p1), p2(p2), color(c) {
    }
};

struct Path {
    std::string d;
    wxColour stroke;
    int strokeWidth;
    bool fill;
    Path(std::string d = "", wxColour s = wxColour(0, 0, 0), int sw = 1, bool f = false)
        : d(d), stroke(s), strokeWidth(sw), fill(f) {
    }
};

// 使用重命名后的类型
using Shape = std::variant<Line, PolyShape, Circle, Text, Path, ArcShape, BezierShape>;

class CanvasElement
{
public:
    CanvasElement() = default;
    CanvasElement(const wxString& name, const wxPoint& pos);
    void Draw(wxDC& dc) const;
    void AddShape(const Shape& shape) { m_shapes.push_back(shape); }
    void AddInputPin(const Point& p, const wxString& name) { m_inputPins.push_back(Pin(p, name, true)); }
    void AddOutputPin(const Point& p, const wxString& name) { m_outputPins.push_back(Pin(p, name, false)); }
    void SetPos(const wxPoint& p) { m_pos = p; }

    const wxString& GetName() const { return m_name; }
    const wxPoint& GetPos() const { return m_pos; }
    const std::vector<Shape>& GetShapes() const { return m_shapes; }
    const std::vector<Pin>& GetInputPins() const { return m_inputPins; }
    const std::vector<Pin>& GetOutputPins() const { return m_outputPins; }

    wxRect GetBounds() const;

    // 添加状态管理方法
    void ToggleState() { m_state = !m_state; }
    bool GetState() const { return m_state; }
    void SetState(bool state) { m_state = state; }

    // 设置元件ID用于识别Pin_Input
    void SetId(const wxString& id) { m_id = id; }
    const wxString& GetId() const { return m_id; }

    // 状态控制方法
    void SetOutputState(int state);  // 0:X, 1:0, 2:1
    int GetOutputState() const { return m_outputState; }

private:
    wxString m_name;
    wxPoint m_pos;
    std::vector<Shape> m_shapes;
    std::vector<Pin> m_inputPins;
    std::vector<Pin> m_outputPins;
    void DrawVector(wxGCDC& gcdc) const;
    std::vector<wxPoint> CalculateBezier(const Point& p0, const Point& p1, const Point& p2, int segments = 16) const;
    // 回退绘制方法
    void DrawFallback(wxDC& dc) const;
    // 1. 正确声明 wxGCDC 版本的 DrawPathFallback（别注释！）
    void DrawPathFallback(wxGCDC& gcdc, const Path& arg, std::function<wxPoint(const Point&)> off) const;
    // 2. 保留 wxDC 版本的 DrawPathFallback
    void DrawPathFallback(wxDC& dc, const Path& arg, std::function<wxPoint(const Point&)> off) const;
    // 添加状态和ID成员
    bool m_state = false; // 默认状态为0/false
    wxString m_id;        // 元件ID
    int m_outputState = 0; // Pin_Output状态：0=X, 1=0, 2=1
};