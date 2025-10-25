#ifdef Polygon
#undef Polygon
#endif
#include "CanvasElement.h"
#include <variant>
#include "my_log.h"

CanvasElement::CanvasElement(const wxString& name, const wxPoint& pos)
    : m_name(name), m_pos(pos)
{
}

void CanvasElement::Draw(wxDC& dc) const
{
    /*MyLog("CanvasElement: Draw %s at %d,%d\n",
        m_name.ToUTF8().data(), m_pos.x, m_pos.y);*/

    // ͳһƽ�� lambda
    auto off = [&](const Point& p)
        {
            return wxPoint(m_pos.x + p.x, m_pos.y + p.y);
        };

    // 1. ������ͼ��
    for (const auto& shape : m_shapes)
    {
        std::visit([&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, PolyShape>)
                {
                    std::vector<wxPoint> pts;
                    for (auto& p : arg.pts) pts.push_back(off(p));
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);      // �ڲ�͸��
                    dc.SetPen(wxPen(arg.color, 2));         // �����ɫ+�߿�
                    dc.DrawPolygon(pts.size(), pts.data());
                }
                else if constexpr (std::is_same_v<T, Line>)
                {
                    dc.SetPen(wxPen(arg.color, 1));
                    dc.DrawLine(off(arg.start), off(arg.end));
                }
                else if constexpr (std::is_same_v<T, Circle>)
                {
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);      // �ڲ�͸��
                    dc.SetPen(wxPen(arg.color, 1));
                    dc.DrawCircle(off(arg.center), arg.radius);
                }
                else if constexpr (std::is_same_v<T, Text>)
                {
                    dc.SetTextForeground(arg.color);
                    dc.SetFont(wxFont(arg.fontSize, wxFONTFAMILY_DEFAULT,
                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
                    dc.DrawText(arg.text, off(arg.pos));
                }
            }, shape);
    }

    // �������ţ�ֻ��ֱ�ߣ���СԲ�㣬ͻ�� 1 ����
    for (const auto& pin : m_inputPins) {
        wxPoint p = off(pin.pos);
        dc.SetPen(wxPen(wxColour(0, 0, 0), 1));
        dc.DrawLine(p, wxPoint(p.x - 1, p.y));   // ͻ������ = 1 ����
    }

    // ������ţ�ֻ��ֱ�ߣ���СԲ�㣬ͻ�� 1 ����
    for (const auto& pin : m_outputPins) {
        wxPoint p = off(pin.pos);
        dc.SetPen(wxPen(wxColour(0, 0, 0), 1));
        dc.DrawLine(p, wxPoint(p.x + 1, p.y));   // ͻ������ = 1 ����
    }
}

#include <algorithm>
#include <limits>

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
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, PolyShape>) {
                for (auto& pt : arg.pts) update(pt);
            }
            else if constexpr (std::is_same_v<T, Line>) {
                update(arg.start); update(arg.end);
            }
            else if constexpr (std::is_same_v<T, Circle>) {
                update({ arg.center.x - arg.radius, arg.center.y - arg.radius });
                update({ arg.center.x + arg.radius, arg.center.y + arg.radius });
            }
            else if constexpr (std::is_same_v<T, Text>) {
                update(arg.pos);
            }
            }, shape);
    }
    return wxRect(minX, minY, maxX - minX, maxY - minY);
}