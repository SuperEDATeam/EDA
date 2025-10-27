#ifdef Polygon
#undef Polygon
#endif

#include "CanvasElement.h"
#include <variant>
#include <cmath>
#include <algorithm>
#include <limits>

// ���������߼��㺯��
std::vector<wxPoint> CanvasElement::CalculateBezier(const Point& p0, const Point& p1, const Point& p2, int segments) const
{
    std::vector<wxPoint> pts;
    for (int i = 0; i <= segments; ++i) {
        double t = double(i) / segments;
        double x = (1 - t) * (1 - t) * p0.x + 2 * (1 - t) * t * p1.x + t * t * p2.x;
        double y = (1 - t) * (1 - t) * p0.y + 2 * (1 - t) * t * p1.y + t * t * p2.y;
        pts.emplace_back(static_cast<int>(x), static_cast<int>(y));
    }
    return pts;
}

CanvasElement::CanvasElement(const wxString& name, const wxPoint& pos)
    : m_name(name), m_pos(pos)
{
}

void CanvasElement::Draw(wxDC& dc) const
{
    auto off = [&](const Point& p) {
        return wxPoint(m_pos.x + p.x, m_pos.y + p.y);
        };

    // ������ͼ��
    for (const auto& shape : m_shapes)
    {
        // ʹ�ø����ݵķ��ʷ�ʽ
        auto visitor = [&](const auto& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, PolyShape>) {
                std::vector<wxPoint> pts;
                for (const auto& p : arg.pts) {
                    pts.push_back(off(p));
                }
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
                dc.SetPen(wxPen(arg.color, 2));
                if (!pts.empty()) {
                    dc.DrawPolygon(static_cast<int>(pts.size()), pts.data());
                }
            }
            else if constexpr (std::is_same_v<T, Line>) {
                // ʹ��Line��������ɫ����
                dc.SetPen(wxPen(arg.color, 2));
                dc.DrawLine(off(arg.start), off(arg.end));
            }
            else if constexpr (std::is_same_v<T, Circle>) {
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
                dc.SetPen(wxPen(arg.color, 2));
                dc.DrawCircle(off(arg.center), arg.radius);
            }
            else if constexpr (std::is_same_v<T, Text>) {
                dc.SetTextForeground(arg.color);
                dc.SetFont(wxFont(arg.fontSize, wxFONTFAMILY_DEFAULT,
                    wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
                dc.DrawText(arg.text, off(arg.pos));
            }
            else if constexpr (std::is_same_v<T, ArcShape>) {
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
                dc.SetPen(wxPen(arg.color, 2));

                double startRad = arg.startAngle * M_PI / 180.0;
                double endRad = arg.endAngle * M_PI / 180.0;

                std::vector<wxPoint> arcPoints;
                int segments = 16;
                for (int i = 0; i <= segments; ++i) {
                    double t = double(i) / segments;
                    double angle = startRad + t * (endRad - startRad);
                    int x = arg.center.x + static_cast<int>(arg.radius * cos(angle));
                    int y = arg.center.y + static_cast<int>(arg.radius * sin(angle));
                    arcPoints.push_back(off(Point(x, y)));
                }

                if (arcPoints.size() >= 2) {
                    dc.DrawLines(static_cast<int>(arcPoints.size()), arcPoints.data());
                }
            }
            else if constexpr (std::is_same_v<T, BezierShape>) {
                dc.SetPen(wxPen(arg.color, 2));
                auto bezierPoints = CalculateBezier(arg.p0, arg.p1, arg.p2);

                std::vector<wxPoint> screenPoints;
                for (const auto& wp : bezierPoints) {
                    // �� wxPoint ת��Ϊ Point��Ȼ���ٵ��� off
                    Point p{ wp.x, wp.y };
                    screenPoints.push_back(off(p));
                }

                if (!screenPoints.empty()) {
                    dc.DrawLines(static_cast<int>(screenPoints.size()), screenPoints.data());
                }
            }
            else if constexpr (std::is_same_v<T, Path>) {
                dc.SetPen(wxPen(arg.stroke, arg.strokeWidth));
                dc.SetBrush(arg.fill ? wxBrush(arg.stroke) : *wxTRANSPARENT_BRUSH);

                std::string d = arg.d;
                if (d.find("A 16 28") != std::string::npos) {
                    // ԭ�е�AND�Ż����߼�
                    int verticalLineLength = 40;
                    int radius = verticalLineLength / 2;

                    // ���������ֱ��
                    dc.DrawLine(off(Point(10, 12)), off(Point(10, 52)));

                    // ������ˮƽ��
                    dc.DrawLine(off(Point(10, 12)), off(Point(10 + radius, 12)));

                    // ������ˮƽ��
                    dc.DrawLine(off(Point(10, 52)), off(Point(10 + radius, 52)));

                    // �����Ҳ��Բ
                    std::vector<wxPoint> arcPoints;
                    for (int angle = -90; angle <= 90; angle += 5) {
                        double rad = angle * M_PI / 180.0;
                        int x = 30 + static_cast<int>(radius * cos(rad));
                        int y = 32 + static_cast<int>(radius * sin(rad));
                        arcPoints.push_back(off(Point(x, y)));
                    }

                    // ���ư�Բ
                    if (arcPoints.size() >= 2) {
                        for (size_t i = 1; i < arcPoints.size(); i++) {
                            dc.DrawLine(arcPoints[i - 1], arcPoints[i]);
                        }
                    }
                }
            }
            };

        std::visit(visitor, shape);
    }

    // 绘制输入引脚（左边，蓝色）
    for (const auto& pin : m_inputPins) {
        wxPoint p = off(pin.pos);
        dc.SetPen(wxPen(wxColour(0, 0, 255), 2)); // 蓝色 (RGB: 0,0,255)
        dc.DrawLine(p, wxPoint(p.x - 6, p.y));
    }

    // 绘制输出引脚（右边，红色）
    for (const auto& pin : m_outputPins) {
        wxPoint p = off(pin.pos);
        dc.SetPen(wxPen(wxColour(255, 0, 0), 2)); // 红色 (RGB: 255,0,0)
        dc.DrawLine(p, wxPoint(p.x + 6, p.y));
    }
}

wxRect CanvasElement::GetBounds() const
{
    if (m_shapes.empty()) return wxRect(m_pos, wxSize(1, 1));

    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int maxY = std::numeric_limits<int>::min();

    auto update = [&](const Point& p) {
        int x = m_pos.x + p.x;
        int y = m_pos.y + p.y;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
        };

    for (const auto& shape : m_shapes) {
        auto visitor = [&](const auto& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, PolyShape>) {
                for (const auto& pt : arg.pts) update(pt);
            }
            else if constexpr (std::is_same_v<T, Line>) {
                update(arg.start);
                update(arg.end);
            }
            else if constexpr (std::is_same_v<T, Circle>) {
                update(Point(arg.center.x - arg.radius, arg.center.y - arg.radius));
                update(Point(arg.center.x + arg.radius, arg.center.y + arg.radius));
            }
            else if constexpr (std::is_same_v<T, Text>) {
                update(arg.pos);
                update(Point(arg.pos.x + 20, arg.pos.y + 10));
            }
            else if constexpr (std::is_same_v<T, ArcShape>) {
                update(Point(arg.center.x - arg.radius, arg.center.y - arg.radius));
                update(Point(arg.center.x + arg.radius, arg.center.y + arg.radius));
            }
            else if constexpr (std::is_same_v<T, BezierShape>) {
                auto pts = CalculateBezier(arg.p0, arg.p1, arg.p2);
                for (const auto& wp : pts) {
                    // �� wxPoint ת��Ϊ Point��Ȼ���ٵ��� update
                    Point p{ wp.x, wp.y };
                    update(p);
                }
            }
            else if constexpr (std::is_same_v<T, Path>) {
                if (arg.d.find("A 16 28") != std::string::npos) {
                    update(Point(10, 12));
                    update(Point(50, 52));
                }
            }
            };

        std::visit(visitor, shape);
    }

    // �������ŵı߽�
    for (const auto& pin : m_inputPins) {
        update(pin.pos);
        update(Point(pin.pos.x - 6, pin.pos.y));
    }

    for (const auto& pin : m_outputPins) {
        update(pin.pos);
        update(Point(pin.pos.x + 6, pin.pos.y));
    }

    return wxRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}