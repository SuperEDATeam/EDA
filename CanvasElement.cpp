#ifdef Polygon
#undef Polygon
#endif
#include "CanvasElement.h"
#include <variant>

CanvasElement::CanvasElement(const wxString& name, const wxPoint& pos)
    : m_name(name), m_pos(pos)
{
}

void CanvasElement::Draw(wxDC& dc) const
{
    // 1. 画主体图形
    for (const auto& shape : m_shapes) {
        std::visit([&dc](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, PolyShape>) {
                std::vector<wxPoint> pts;
                for (auto& p : arg.pts) pts.emplace_back(p.x, p.y);
                dc.DrawPolygon(pts.size(), pts.data());
            }
            else if constexpr (std::is_same_v<T, Line>) {
                dc.DrawLine(arg.start.x, arg.start.y, arg.end.x, arg.end.y);
            }
            else if constexpr (std::is_same_v<T, Circle>) {
                dc.DrawCircle(arg.center.x, arg.center.y, arg.radius);
            }
            else if constexpr (std::is_same_v<T, Text>) {
                dc.SetFont(wxFont(arg.fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
                dc.DrawText(arg.text, arg.pos.x, arg.pos.y);
            }
            }, shape);
    }

    // 2. 画输入引脚
    for (const auto& pin : m_inputPins) {
        dc.SetPen(wxPen(wxColour(0, 0, 0), 1));
        dc.DrawLine(pin.pos.x, pin.pos.y, pin.pos.x - 2, pin.pos.y);   // 向左延伸
        dc.DrawCircle(pin.pos.x - 2, pin.pos.y, 1);                    // 小圆点
    }

    // 3. 画输出引脚
    for (const auto& pin : m_outputPins) {
        dc.SetPen(wxPen(wxColour(0, 0, 0), 1));
        dc.DrawLine(pin.pos.x, pin.pos.y, pin.pos.x + 2, pin.pos.y);   // 向右延伸
        dc.DrawCircle(pin.pos.x + 2, pin.pos.y, 1);                    // 小圆点
    }
}