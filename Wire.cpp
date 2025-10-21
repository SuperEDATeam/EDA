#include "Wire.h"

void Wire::Draw(wxDC& dc) const {
    if (pts.size() < 2) return;
    dc.SetPen(wxPen(*wxBLACK, 2));
    for (size_t i = 1; i < pts.size(); ++i)
        dc.DrawLine(pts[i - 1].pos, pts[i].pos);
}

//采用曼哈顿路由（Manhattan Wiring）代替传统的正交路由
// 输入：起点 a，终点 b   输出：2~3 个点的折线
std::vector<ControlPoint>
Wire::RouteOrtho(const wxPoint& a, const wxPoint& b)
{
    std::vector<ControlPoint> out;
    out.push_back({ a, CPType::Pin });

    if (a.x == b.x || a.y == b.y) {          // 纯横 or 纯竖
        out.push_back({ b, CPType::Free });
        return out;
    }

    // 先横后竖：中间折点在终点正上方（或下方）
    wxPoint bend(b.x, a.y);
    out.push_back({ bend, CPType::Bend });
    out.push_back({ b, CPType::Free });
    return out;
}