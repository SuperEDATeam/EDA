#pragma once
#include <wx/wx.h>
#include <vector>

enum class CPType { Pin, Bend, Free };

struct ControlPoint {
    wxPoint  pos;
    CPType   type = CPType::Free;
    // 若吸附到引脚，可扩展存元件指针/pin索引
};

class Wire {
public:
    std::vector<ControlPoint> pts;

    Wire() = default;
    explicit Wire(std::vector<ControlPoint> v) : pts(std::move(v)) {}

    // 核心接口
    void Draw(wxDC& dc) const;                          // 画线
    void AddPoint(const ControlPoint& cp) { pts.push_back(cp); }
    void Clear() { pts.clear(); }
    bool Empty() const { return pts.empty(); }
    size_t Size() const { return pts.size(); }

    //采用曼哈顿路由（Manhattan Wiring）代替传统的正交路由
    static std::vector<ControlPoint> RouteOrtho(const wxPoint& a, const wxPoint& b);
};