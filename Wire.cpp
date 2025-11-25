#include "Wire.h"
#include "CanvasPanel.h"

void Wire::Draw(wxDC& dc) const {
    if (pts.size() < 2) return;
    dc.SetPen(wxPen(*wxBLACK, 2));
    for (size_t i = 1; i < pts.size(); ++i)
        dc.DrawLine(pts[i - 1].pos, pts[i].pos);
}


std::vector<ControlPoint> Wire::RouteOrtho(
    const ControlPoint& start,
    const ControlPoint& end,
    PinDirection startDir,
    PinDirection endDir) {

    std::vector<ControlPoint> out;

    // 边界情况：起点和终点相同
    if (start.pos == end.pos) {
        out.push_back({ start.pos, start.type });
        out.push_back({ end.pos, end.type });
        return out;
    }

    // 添加起点
    out.push_back({ start.pos, start.type });

    // 曼哈顿路由：先水平后垂直 或 先垂直后水平
    // 这里使用"先水平后垂直"策略
    int med_x = (start.pos.x + end.pos.x) / 2;
    med_x = med_x / 20 * 20;
    wxPoint med_1(med_x, start.pos.y);
    wxPoint med_2(med_x, end.pos.y);

    // 只有当中间点与起点不同时才添加
    if (med_1 != start.pos) {
        out.push_back({ med_1, CPType::Bend });
    }

    if (med_2 != end.pos) {
        out.push_back({ med_2, CPType::Bend });
    }

    // 添加终点
    out.push_back({ end.pos, end.type });

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

std::vector<ControlPoint> Wire::RouteBranch(
    const ControlPoint& branchStart,    // 分支起点（导线上的点）
    const ControlPoint& branchEnd,      // 分支终点
    PinDirection endDir) {

    std::vector<ControlPoint> out;

    // 添加分支起点
    out.push_back({ branchStart.pos, CPType::Bend });

    // 分支路由策略：先垂直偏移一段距离，再水平连接到目标
    int offsetY = 20; // 垂直偏移量

    // 第一个中间点：垂直偏移
    wxPoint mid1(branchStart.pos.x, branchStart.pos.y + offsetY);
    out.push_back({ mid1, CPType::Bend });

    // 第二个中间点：水平对齐到终点
    wxPoint mid2(branchEnd.pos.x, branchStart.pos.y + offsetY);
    out.push_back({ mid2, CPType::Bend });

    // 添加终点
    out.push_back({ branchEnd.pos, CPType::Bend });

    return out;
}