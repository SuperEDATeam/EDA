#include "Wire.h"

void Wire::Draw(wxDC& dc) const {
    if (pts.size() < 2) return;
    dc.SetPen(wxPen(*wxBLACK, 2));
    for (size_t i = 1; i < pts.size(); ++i)
        dc.DrawLine(pts[i - 1].pos, pts[i].pos);
}

std::vector<ControlPoint> Wire::RouteOrtho(const wxPoint& a, const wxPoint& b) {
    std::vector<ControlPoint> out;
    out.push_back({ a, CPType::Pin });
    if (a.x == b.x || a.y == b.y) {              // Ö±Ïß
        out.push_back({ b, CPType::Free });
    }
    else {                                       // L ÐÎ
        wxPoint bend((a.x + b.x) / 2, a.y);
        out.push_back({ bend, CPType::Bend });
        out.push_back({ b, CPType::Free });
    }
    return out;
}