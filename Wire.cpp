#include "Wire.h"
#include "CanvasPanel.h"

void Wire::Draw(wxDC& dc) const {
    if (pts.size() < 2) return;
    for (size_t i = 1; i < pts.size() + 1; ++i) {
        
        if (i != pts.size()) {
            if (status == false) dc.SetPen(wxPen(colors[0], 2));
            else dc.SetPen(wxPen(colors[1], 2));
            dc.DrawLine(pts[i - 1].pos, pts[i].pos);
        }
        switch (pts[i - 1].type) {
        case CPType::Pin:
            break;
        case CPType::Branch:
            dc.SetPen(wxPen(colors[2], 1));
            dc.SetBrush(wxBrush(colors[2], wxBRUSHSTYLE_SOLID));
            dc.DrawCircle(pts[i - 1].pos, 3);
            break;
        case CPType::Free:
            if (status == false) {
                dc.SetPen(wxPen(colors[0], 2));
                dc.SetBrush(wxBrush(colors[0], wxBRUSHSTYLE_SOLID));
            }
            else {
                dc.SetPen(wxPen(colors[1], 2));
                dc.SetBrush(wxBrush(colors[1], wxBRUSHSTYLE_SOLID));
            }
            
            dc.DrawCircle(pts[i - 1].pos, 3);
            break;
        }
    }
}

void Wire::DrawColor(wxDC& dc) const {
    if (pts.size() < 2) return;
    dc.SetPen(wxPen(wxColor(44, 145, 224), 2));
    for (size_t i = 1; i < pts.size(); ++i)
        dc.DrawLine(pts[i - 1].pos, pts[i].pos);

    dc.SetPen(wxPen(wxColor(44, 145, 224, 32), 8));
    for (size_t i = 1; i < pts.size(); ++i)
        dc.DrawLine(pts[i - 1].pos, pts[i].pos);
}


void Wire::GenerateCells()
{
    cells.clear();
    if (pts.size() < 2) return;

    for (int i = 1; i < pts.size(); ++i) {
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
            if (cells.empty() || cell != cells.back().pos)
                cells.push_back({cell, CellType::Norm, i-1, i});
        }
    }
    for (int i = 1; i < pts.size(); ++i) {
        wxPoint p0 = pts[i - 1].pos;
        wxPoint p1 = pts[i].pos;
        wxPoint pos = p0 + (p1 - p0) / 2;
        cells.push_back({ pos , CellType::Mid , i-1 , i});
    }
}


std::vector<ControlPoint> Wire::Route(const ControlPoint& start, const ControlPoint& end) {
    std::vector<ControlPoint> out;

    // 边界情况：起点和终点相同
    if (start.pos == end.pos) {
        out.push_back({ start.pos, start.type });
        out.push_back({ end.pos, end.type });
        return out;
    }

    // 添加起点
    out.push_back({ start.pos, start.type });

    // 处理自由点的情况
    CPType effectiveStartType = start.type;
    CPType effectiveEndType = end.type;

    // 如果有一个是自由点，根据另一个点的类型确定行为
    if (start.type == CPType::Free) {
        if (end.type == CPType::Free) {
            effectiveStartType = CPType::Pin;
            effectiveEndType = CPType::Pin;
        }
        else {
            effectiveStartType = (end.type == CPType::Pin) ? CPType::Pin : CPType::Branch;
        }
    }
    if (end.type == CPType::Free) {
        if (start.type == CPType::Free) {
            effectiveStartType = CPType::Pin;
            effectiveEndType = CPType::Pin;
        }
        else {
            effectiveEndType = (start.type == CPType::Pin) ? CPType::Pin : CPType::Branch;
        }
    }

    // 根据标准化后的类型进行路由
    if (effectiveStartType == CPType::Branch && effectiveEndType == CPType::Branch) {
        // 分支点到分支点：先竖直，再水平，再竖直，平分竖直落差
        int mid_y1 = start.pos.y + (end.pos.y - start.pos.y) / 2;
        int mid_y2 = start.pos.y + (end.pos.y - start.pos.y) / 2;

        // 对齐到网格
        mid_y1 = mid_y1 / 20 * 20;
        mid_y2 = mid_y2 / 20 * 20;

        wxPoint vert_1(start.pos.x, mid_y1);
        wxPoint horz_1(end.pos.x, mid_y1);
        wxPoint vert_2(end.pos.x, mid_y2);

        if (vert_1 != start.pos) {
            out.push_back({ vert_1, CPType::Bend });
        }
        if (horz_1 != vert_1) {
            out.push_back({ horz_1, CPType::Bend });
        }
        if (vert_2 != horz_1) {
            out.push_back({ vert_2, CPType::Bend });
        }
    }
    else if (effectiveStartType == CPType::Pin && effectiveEndType == CPType::Pin) {
        // 引脚到引脚：先水平，再竖直，再水平，平分水平差
        int mid_x1 = start.pos.x + (end.pos.x - start.pos.x) / 2;
        int mid_x2 = start.pos.x + (end.pos.x - start.pos.x) / 2;

        // 对齐到网格
        mid_x1 = mid_x1 / 20 * 20;
        mid_x2 = mid_x2 / 20 * 20;

        wxPoint horz_1(mid_x1, start.pos.y);
        wxPoint vert_1(mid_x1, end.pos.y);
        wxPoint horz_2(mid_x2, end.pos.y);

        if (horz_1 != start.pos) {
            out.push_back({ horz_1, CPType::Bend });
        }
        if (vert_1 != horz_1) {
            out.push_back({ vert_1, CPType::Bend });
        }
        if (horz_2 != vert_1) {
            out.push_back({ horz_2, CPType::Bend });
        }
    }
    else if (effectiveStartType == CPType::Branch && effectiveEndType == CPType::Pin) {
        // 分支到引脚：先竖直再水平
        wxPoint med(start.pos.x, end.pos.y);
        med.y = med.y / 20 * 20; // 对齐到网格

        if (med != start.pos && med != end.pos) {
            out.push_back({ med, CPType::Bend });
        }
    }
    else if (effectiveStartType == CPType::Pin && effectiveEndType == CPType::Branch) {
        // 引脚到分支：先水平再竖直
        wxPoint med(end.pos.x, start.pos.y);
        med.x = med.x / 20 * 20; // 对齐到网格

        if (med != start.pos && med != end.pos) {
            out.push_back({ med, CPType::Bend });
        }
    }
    else {
        // 默认情况：使用正交路由作为后备
        int med_x = (start.pos.x + end.pos.x) / 2;
        med_x = med_x / 20 * 20;
        wxPoint med_1(med_x, start.pos.y);
        wxPoint med_2(med_x, end.pos.y);

        if (med_1 != start.pos) {
            out.push_back({ med_1, CPType::Bend });
        }
        if (med_2 != end.pos) {
            out.push_back({ med_2, CPType::Bend });
        }
    }

    // 添加终点
    out.push_back({ end.pos, end.type });

    return out;
}

wxRect Wire::GetBounds() const{
    return wxRect(pts.front().pos, pts.back().pos);
}