#include "Wire.h"

void Wire::Draw(wxDC& dc) const {
    if (pts.size() < 2) return;
    dc.SetPen(wxPen(*wxBLACK, 2));
    for (size_t i = 1; i < pts.size(); ++i)
        dc.DrawLine(pts[i - 1].pos, pts[i].pos);
}

//采用曼哈顿路由（Manhattan Wiring）代替传统的正交路由
// 输入：起点 a，终点 b   输出：2~3 个点的折线
// cjc修改过
std::vector<ControlPoint> Wire::RouteOrtho(const wxPoint& a, const wxPoint& b) {
    std::vector<ControlPoint> out;
    out.push_back({ a, CPType::Pin });

    // 如果起点和终点在同一位置，直接返回
    if (a == b) {
        out.push_back({ b, CPType::Free });
        return out;
    }

    // 如果水平或垂直对齐，直接连接
    if (a.x == b.x || a.y == b.y) {
        out.push_back({ b, CPType::Free });
        return out;
    }

    // 曼哈顿路由：选择更合理的拐点
    // 计算水平和垂直距离
    int dx = b.x - a.x;
    int dy = b.y - a.y;

    // 选择拐点位置，尽量让导线路径自然
    // 如果水平距离较大，先水平后垂直
    if (abs(dx) > abs(dy)) {
        wxPoint bend1(a.x + dx / 2, a.y);
        wxPoint bend2(a.x + dx / 2, b.y);
        out.push_back({ bend1, CPType::Bend });
        out.push_back({ bend2, CPType::Bend });
    }
    // 如果垂直距离较大，先垂直后水平
    else {
        wxPoint bend1(a.x, a.y + dy / 2);
        wxPoint bend2(b.x, a.y + dy / 2);
        out.push_back({ bend1, CPType::Bend });
        out.push_back({ bend2, CPType::Bend });
    }

    out.push_back({ b, CPType::Free });
    return out;
}

void Wire::GenerateCells()
{
    cells.clear();
    if (pts.size() < 2) return;

    for (size_t i = 1; i < pts.size(); ++i) {
        wxPoint p0 = pts[i - 1].pos;
        wxPoint p1 = pts[i].pos;
        int dx = abs(p1.x - p0.x);
        int dy = abs(p1.y - p0.y);
        int steps = std::max(dx, dy) / 5;          // 每 2 px 一格
        if (steps == 0) steps = 1;
        for (int s = 0; s <= steps; ++s) {
            double t = double(s) / steps;
            wxPoint cell((1 - t) * p0.x + t * p1.x,
                (1 - t) * p0.y + t * p1.y);
            // 去重（可选）
            if (cells.empty() || cell != cells.back())
                cells.push_back(cell);
        }
    }
}