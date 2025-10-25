#include "Wire.h"

void Wire::Draw(wxDC& dc) const {
    if (pts.size() < 2) return;
    dc.SetPen(wxPen(*wxBLACK, 2));
    for (size_t i = 1; i < pts.size(); ++i)
        dc.DrawLine(pts[i - 1].pos, pts[i].pos);
}

//����������·�ɣ�Manhattan Wiring�����洫ͳ������·��
// ���룺��� a���յ� b   �����2~3 ���������
std::vector<ControlPoint>
Wire::RouteOrtho(const wxPoint& a, const wxPoint& b)
{
    std::vector<ControlPoint> out;
    out.push_back({ a, CPType::Pin });

    if (a.x == b.x || a.y == b.y) {          // ���� or ����
        out.push_back({ b, CPType::Free });
        return out;
    }

    // �Ⱥ�������м��۵����յ����Ϸ������·���
    wxPoint bend(b.x, a.y);
    out.push_back({ bend, CPType::Bend });
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
        int steps = std::max(dx, dy) / 5;          // ÿ 2 px һ��
        if (steps == 0) steps = 1;
        for (int s = 0; s <= steps; ++s) {
            double t = double(s) / steps;
            wxPoint cell((1 - t) * p0.x + t * p1.x,
                (1 - t) * p0.y + t * p1.y);
            // ȥ�أ���ѡ��
            if (cells.empty() || cell != cells.back())
                cells.push_back(cell);
        }
    }
}