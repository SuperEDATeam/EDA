#pragma once
#include <wx/wx.h>
#include <wx/dcgraph.h>  
#include <wx/graphics.h>  
#include <vector>
#include <variant>
#include <map>

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

// 最高基类 CanvasElement，管理形状，位置，旋转，轮廓，HitTest，绘制等基本功能
class CanvasElement {
private:
    wxString m_id; // 元素ID，名称
    int g_id;      // 全局ID，用于标识元素的唯一性

    wxPoint center; // 用于旋转的中心点
    wxRect m_bounds; // 元素的边界矩形，用于HitTest
    std::vector<Shape> m_shapes; // 形状集合

    wxPoint m_pos;    // 位置
    int m_rotation = 0; // 旋转角度

public:
    CanvasElement() = default;
    // 元素ID相关
    void SetId(const wxString& id) { m_id = id; }
    wxString GetId() const { return m_id; }
    void SetGlobalId(int id) { g_id = id; }
    int GetGlobalId() const { return g_id; }

    // 元素形状相关
    void SetCenter(const wxPoint& center) { this->center = center; }
    wxPoint GetCenter() const { return center; }
    void SetBounds(const wxRect& bounds) { m_bounds = bounds; }
    wxRect GetBounds() const { return m_bounds; }
    void AddShape(const Shape& shape) { m_shapes.push_back(shape); }
    const std::vector<Shape>& GetShapes() const { return m_shapes; }

    // 元素摆放相关
    void SetPos(const wxPoint& p) { m_pos = p; }
    const wxPoint& GetPos() const { return m_pos; }
    void SetRotation(int rotation) { m_rotation = rotation % 360; }
    int GetRotation() const { return m_rotation; }


    // 元素操作相关
    bool HitTest(const wxPoint& pos) { return m_bounds.Contains(pos); };
    void Draw(wxDC& dc) const;
    void Draw(wxGCDC& gcdc) const;
};

enum class SimState {
    INVALID = 0,     // X - 无效/未连接
    LOW = 1,         // 0 - 低电平
    HIGH = 2,        // 1 - 高电平
    UNKNOWN = 3,     // ? - 未知
    HIGH_Z = 4       // Z - 高阻态
};


struct Pin {
    Point pos;
    wxString name;
    bool isInput;
    Pin(Point p = Point(), wxString n = "", bool input = true)
        : pos(p), name(n), isInput(input) {
    }
};


struct TimingProperties {
    double riseDelay = 0.0;      // 上升延迟 (ns)
    double fallDelay = 0.0;      // 下降延迟 (ns)
    double setupTime = 0.0;      // 建立时间 (ns)
    double holdTime = 0.0;       // 保持时间 (ns)
    double minPulseWidth = 0.0;  // 最小脉冲宽度 (ns)
};

struct ElectricalProperties {
    double staticPower = 0.0;        // 静态功耗 (mW)
    double powerPerTransition = 0.0; // 每次跳变功耗 (nJ)
    double inputCapacitance = 0.0;   // 输入电容 (pF)
    double driveStrength = 1.0;      // 驱动强度（归一化）
    int maxFanout = 10;             // 最大扇出
};

struct PhysicalProperties {
    double estimatedArea = 0.0;      // 预估面积 (mm²)
    wxSize boundingBox;              // 逻辑包围盒
    std::map<size_t, wxPoint> pinPositions; // 引脚物理位置
};








class SimulationElement {
private:


};


class AnnotationElement {

};