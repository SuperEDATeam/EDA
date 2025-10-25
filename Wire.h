#pragma once
#include <wx/wx.h>
#include <vector>

enum class CPType { Pin, Bend, Free };

struct ControlPoint {
    wxPoint  pos;
    CPType   type = CPType::Free;
    // 若吸附到引脚，可扩展存元件指针/pin索引
};

/* 导线-导线连接记录 */
struct WireWireAnchor {
    size_t srcWire;   // 新导线索引
    size_t srcPt;     // 0 或 pts.size()-1
    size_t dstWire;   // 被连接的导线索引
    size_t dstCell;   // 被连接的小方块序号
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

    std::vector<wxPoint> cells;          // 每 2 px 小格中心
    void GenerateCells();                // 一次性切分
private:

};