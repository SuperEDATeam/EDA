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
    PinDirection endDir)
{
    std::vector<ControlPoint> out;
    out.push_back({ start.pos, start.type });

    // 边界情况处理
    if (start.pos == end.pos) {
        out.push_back({ end.pos, end.type });
        return out;
    }

    //// 第一步：从起始引脚水平延伸
    //wxPoint startExit = CalculateHorizontalExit(start.pos, startDir);
    //out.push_back({ startExit, CPType::Bend });

    //// 第二步：从终止引脚水平延伸  
    //wxPoint endExit = CalculateHorizontalExit(end.pos, endDir);

    // 第三步：连接两个延伸点
    ConnectExits(out, start.pos, end.pos);

    // 第四步：连接到终止引脚
    out.push_back({ end.pos, CPType::Bend });
    out.push_back({ end.pos, end.type });

    return out;
}


wxPoint Wire::CalculateHorizontalExit(const wxPoint& pinPos, PinDirection dir) {
    const int EXIT_DISTANCE = 20; // 水平延伸距离

    switch (dir) {
    case PinDirection::Left:
    case PinDirection::Right:
        // 水平引脚：直接水平延伸
        return wxPoint(
            pinPos.x + (dir == PinDirection::Right ? EXIT_DISTANCE : -EXIT_DISTANCE),
            pinPos.y
        );

    case PinDirection::Up:
    case PinDirection::Down:
        // 垂直引脚：先垂直移动，再水平延伸
        int verticalOffset = (dir == PinDirection::Down ? EXIT_DISTANCE : -EXIT_DISTANCE);
        return wxPoint(pinPos.x, pinPos.y + verticalOffset);
    }
    return pinPos;
}

void Wire::ConnectExits(std::vector<ControlPoint>& path,
    const wxPoint& startExit, const wxPoint& endExit) {

    // 情况1：两个延伸点在同一水平线上
    if (startExit.y == endExit.y) {
        // 直接水平连接
        path.push_back({ endExit, CPType::Free });
        return;
    }

    // 情况2：需要创建拐点连接
    // 策略：先垂直移动到中间位置，再水平移动，再垂直移动

    int midY = (startExit.y + endExit.y) / 2;

    // 第一个垂直移动
    wxPoint verticalPoint(startExit.x, midY);
    path.push_back({ verticalPoint, CPType::Bend });

    // 水平移动
    wxPoint horizontalPoint(endExit.x, midY);
    path.push_back({ horizontalPoint, CPType::Bend });

    // 第二个垂直移动已经在后续步骤中处理
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