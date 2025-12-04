#pragma once
#include <wx/wx.h>
#include <vector>
#include <tuple>

class CanvasPanel;

enum class CPType { Pin, Bend, Free, Branch };

struct ControlPoint {
    wxPoint  pos;
    CPType   type = CPType::Free;
    // 若吸附到引脚，可扩展存元件指针/pin索引
};

struct MidCell {
    wxPoint  pos;
    int pre_pts_idx;
    int next_pts_idx;
};

class Wire {
public:
    std::vector<ControlPoint> pts;

    Wire() = default;
    explicit Wire(std::vector<ControlPoint> v) : pts(std::move(v)) {}

    // 核心接口
    void Draw(wxDC& dc) const;                          // 画线
    void DrawColor(wxDC& dc) const;
    void AddPoint(const ControlPoint& cp) { pts.push_back(cp); }
    void Clear() { pts.clear(); }
    bool Empty() const { return pts.empty(); }
    size_t Size() const { return pts.size(); }

    wxRect GetBounds() const;

    CanvasPanel* m_canvas;
    std::vector<wxPoint> cells;          // 每 2 px 小格中心
    std::vector <MidCell> midCells;
    void GenerateCells();                // 一次性切分
    bool status = false;                 // bool状态
    void SetStatus(bool s) { status = s; }
    bool GetStatus() const { return status; }
    
    std::vector<wxColor> colors = { wxColour(0, 128, 0), wxColour(0, 255, 0), *wxBLUE, *wxRED };         // 颜色序列:导线0颜色，导线1颜色，导线分支点颜色, 自由点颜色

public:
    // ... 现有成员 ...

    // 分支相关

    static std::vector<ControlPoint> Route(const ControlPoint& start, const ControlPoint& end);
};