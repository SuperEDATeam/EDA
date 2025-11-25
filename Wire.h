#pragma once
#include <wx/wx.h>
#include <vector>

class CanvasPanel;

enum class CPType { Pin, Bend, Free, Branch };

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

    CanvasPanel* m_canvas;
    std::vector<wxPoint> cells;          // 每 2 px 小格中心
    void GenerateCells();                // 一次性切分
public:
    // ... 现有成员 ...

    // 分支相关

    static std::vector<ControlPoint> Route(const ControlPoint& start, const ControlPoint& end);
};