#ifdef Polygon
#undef Polygon
#endif

#include "CanvasElement.h"
#include <variant>
#include <cmath>
#include <algorithm>
#include <limits>
#include <wx/dcgraph.h>
#include <wx/graphics.h>

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
    // 优先使用矢量绘制
    if (auto gcdc = dynamic_cast<wxGCDC*>(&dc)) {
        DrawVector(*gcdc);
    }
    else {
        DrawFallback(dc); // 兼容回退
    }
}

void CanvasElement::DrawVector(wxGCDC& gcdc) const
{
    wxGraphicsContext* gc = gcdc.GetGraphicsContext();
    if (!gc) return;

    wxGraphicsMatrix origMatrix = gc->GetTransform();

    // 2. 提取平移和缩放（和之前一样，只是用对象调用 Get()）
    double a = 0.0, b = 0.0, c = 0.0, d = 0.0;
    double origTx = 0.0, origTy = 0.0;
    origMatrix.Get(&a, &b, &c, &d, &origTx, &origTy); // 对象调用 Get()
    double origSx = a;
    double origSy = d;
    gc->Translate(m_pos.x, m_pos.y);

    // -------------------------- 3. 绘制所有形状（逻辑不变，补充 Path 分支避免 visit 遗漏） --------------------------
    for (const auto& shape : m_shapes) {
        std::visit([&](auto&& s) {
            using T = std::decay_t<decltype(s)>;

            // 对于Pin_Input元件，根据状态修改绘制
            if (m_id == "Pin_Input") {
                if constexpr (std::is_same_v<T, Circle>) {
                    // 根据状态选择颜色
                    wxColour circleColor = m_state ? wxColour(0, 128, 0) : wxColour(0, 255, 0); // 深绿色 : 绿色
                    wxColour fillColor = m_state ? wxColour(0, 128, 0) : wxColour(0, 255, 0);

                    gc->SetPen(wxPen(circleColor, 3.0));
                    gc->SetBrush(wxBrush(fillColor));
                    gc->DrawEllipse(s.center.x - s.radius, s.center.y - s.radius,
                        s.radius * 2, s.radius * 2);
                    return; // 跳过原始绘制
                }
                else if constexpr (std::is_same_v<T, Text>) {
                    // 根据状态显示不同的文本
                    wxString displayText = m_state ? "1" : "0";
                    wxFont font(s.fontSize, wxFONTFAMILY_DEFAULT,
                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
                    gc->SetFont(font, *wxWHITE); // 保持白色文本
                    gc->DrawText(displayText, s.pos.x, s.pos.y);
                    return; // 跳过原始绘制
                }
            }
            // 绘制Pin_Output元件，根据状态显示不同内容
            // 在CanvasElement.cpp的DrawVector方法中，在Pin_Input的绘制逻辑后添加Pin_Output的处理：

            if (m_id == "Pin_Output") {
                if constexpr (std::is_same_v<T, Circle>) {
                    // 根据半径区分外圈和内圈
                    if (s.radius == 23) {
                        // 外圈：保持原始绘制（不填充）
                        gc->SetPen(wxPen(s.color, 3.0));
                        gc->SetBrush(*wxTRANSPARENT_BRUSH);
                        gc->DrawEllipse(s.center.x - s.radius, s.center.y - s.radius,
                            s.radius * 2, s.radius * 2);
                        // 不返回，继续处理内圈
                    }
                    else if (s.radius == 13) {
                        // 内圈：根据状态绘制
                        wxColour circleColor, fillColor;
                        wxString displayText;

                        switch (m_outputState) {
                        case 1: // 状态0
                            circleColor = wxColour(0, 255, 0); // 绿色
                            fillColor = wxColour(0, 255, 0);
                            displayText = "0";
                            break;
                        case 2: // 状态1
                            circleColor = wxColour(0, 128, 0); // 深绿色
                            fillColor = wxColour(0, 128, 0);
                            displayText = "1";
                            break;
                        default: // 状态X（默认）
                            circleColor = wxColour(0xCC, 0xE0, 0x99); // #CCE099
                            fillColor = wxColour(0xCC, 0xE0, 0x99);
                            displayText = "X";
                            break;
                        }

                        gc->SetPen(wxPen(circleColor, 3.0));
                        gc->SetBrush(wxBrush(fillColor));
                        gc->DrawEllipse(s.center.x - s.radius, s.center.y - s.radius,
                            s.radius * 2, s.radius * 2);
                        return; // 内圈绘制后返回
                    }
                }
                else if constexpr (std::is_same_v<T, Text>) {
                    wxString displayText;
                    switch (m_outputState) {
                    case 1: displayText = "0"; break;
                    case 2: displayText = "1"; break;
                    default: displayText = "X"; break;
                    }

                    wxFont font(s.fontSize, wxFONTFAMILY_DEFAULT,
                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
                    gc->SetFont(font, *wxWHITE);
                    gc->DrawText(displayText, s.pos.x, s.pos.y);
                    return; // 跳过原始绘制
                }
            }


            // 分支1：Line
            if constexpr (std::is_same_v<T, Line>) {
                gc->SetPen(wxPen(s.color, 3.0));
                gc->StrokeLine(s.start.x, s.start.y, s.end.x, s.end.y);
            }
            // 分支2：ArcShape
            else if constexpr (std::is_same_v<T, ArcShape>) {
                gc->SetPen(wxPen(s.color, 3.0));
                wxGraphicsPath path = gc->CreatePath();
                path.AddArc(s.center.x, s.center.y, s.radius,
                    s.startAngle * M_PI / 180, s.endAngle * M_PI / 180, true);
                gc->StrokePath(path);
            }
            // 分支3：Text
            else if constexpr (std::is_same_v<T, Text>) {
                wxFont font(s.fontSize, wxFONTFAMILY_DEFAULT,
                    wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
                gc->SetFont(font, s.color);
                gc->DrawText(s.text, s.pos.x, s.pos.y);
            }
            // 分支4：Circle
            else if constexpr (std::is_same_v<T, Circle>) {
                gc->SetPen(wxPen(s.color, 3.0));
                gc->SetBrush(s.fill ? wxBrush(s.fillColor) : *wxTRANSPARENT_BRUSH);
                gc->DrawEllipse(s.center.x - s.radius, s.center.y - s.radius,
                    s.radius * 2, s.radius * 2);
            }
            // 分支5：PolyShape
            else if constexpr (std::is_same_v<T, PolyShape>) {
                wxGraphicsPath path = gc->CreatePath();
                if (!s.pts.empty()) {
                    path.MoveToPoint(s.pts[0].x, s.pts[0].y);
                    for (size_t i = 1; i < s.pts.size(); ++i) {
                        path.AddLineToPoint(s.pts[i].x, s.pts[i].y);
                    }
                    path.CloseSubpath();
                }
                gc->SetPen(wxPen(s.color, 3.0));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->StrokePath(path);
            }
            // 分支6：BezierShape
            else if constexpr (std::is_same_v<T, BezierShape>) {
                gc->SetPen(wxPen(s.color, 3.0));
                wxGraphicsPath path = gc->CreatePath();
                path.MoveToPoint(s.p0.x, s.p0.y);
                path.AddCurveToPoint(s.p1.x, s.p1.y, s.p1.x, s.p1.y, s.p2.x, s.p2.y);
                gc->StrokePath(path);
            }
            // 分支7：Path（补充完整，避免覆盖不全）
            else if constexpr (std::is_same_v<T, Path>) {
                gc->SetPen(wxPen(s.stroke, s.strokeWidth));
                gc->SetBrush(s.fill ? wxBrush(s.stroke) : *wxTRANSPARENT_BRUSH);
                DrawPathFallback(gcdc, s, [&](const Point& p) { return wxPoint(p.x, p.y); });
            }

            }, shape);  // 确保 std::visit 的 lambda 正确闭合
    }

    // 绘制输入引脚（蓝色圆点）
    for (const auto& pin : m_inputPins) {
        gc->SetPen(wxPen(wxColour(0, 0, 255), 1.0)); // 蓝色边框
        gc->SetBrush(wxBrush(wxColour(0, 0, 255)));  // 蓝色填充
        gc->DrawEllipse(pin.pos.x - 3, pin.pos.y - 3, 6, 6); // 半径为3的圆点
    }

    // 绘制输出引脚（红色圆点）
    for (const auto& pin : m_outputPins) {
        gc->SetPen(wxPen(wxColour(255, 0, 0), 1.0)); // 红色边框
        gc->SetBrush(wxBrush(wxColour(255, 0, 0)));  // 红色填充
        gc->DrawEllipse(pin.pos.x - 3, pin.pos.y - 3, 6, 6); // 半径为3的圆点
    }

    // -------------------------- 4. 恢复原始上下文变换 --------------------------
    gc->SetTransform(origMatrix); // 这里不用 *，直接传对象
}

// 回退绘制方法（当无法使用 GraphicsContext 时）
void CanvasElement::DrawFallback(wxDC& dc) const
{
    auto off = [&](const Point& p) {
        return wxPoint(m_pos.x + p.x, m_pos.y + p.y);
        };

    for (const auto& shape : m_shapes)
    {
        auto visitor = [&](const auto& arg) {
            using T = std::decay_t<decltype(arg)>;

            // 对于Pin_Input元件，根据状态修改绘制
            if (m_id == "Pin_Input") {
                if constexpr (std::is_same_v<T, Circle>) {
                    wxColour circleColor = m_state ? wxColour(0, 128, 0) : wxColour(0, 255, 0);
                    wxColour fillColor = m_state ? wxColour(0, 128, 0) : wxColour(0, 255, 0);

                    if (arg.fill) {
                        dc.SetBrush(wxBrush(fillColor));
                    }
                    else {
                        dc.SetBrush(*wxTRANSPARENT_BRUSH);
                    }
                    dc.SetPen(wxPen(circleColor, 1));
                    dc.DrawCircle(off(arg.center), arg.radius);
                    return;
                }
                else if constexpr (std::is_same_v<T, Text>) {
                    wxString displayText = m_state ? "1" : "0";
                    dc.SetTextForeground(*wxWHITE);
                    dc.SetFont(wxFont(arg.fontSize, wxFONTFAMILY_DEFAULT,
                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
                    dc.DrawText(displayText, off(arg.pos));
                    return;
                }
            }

            if (m_id == "Pin_Output") {
                if constexpr (std::is_same_v<T, Circle>) {
                    // 根据半径区分外圈和内圈
                    if (arg.radius == 23) {
                        // 外圈：保持原始绘制
                        dc.SetBrush(*wxTRANSPARENT_BRUSH);
                        dc.SetPen(wxPen(arg.color, 1));
                        dc.DrawCircle(off(arg.center), arg.radius);
                        return;
                    }
                    else if (arg.radius == 13) {
                        // 内圈：根据状态绘制
                        wxColour circleColor, fillColor;
                        wxString displayText;

                        switch (m_outputState) {
                        case 1: // 状态0
                            circleColor = wxColour(0, 255, 0);
                            fillColor = wxColour(0, 255, 0);
                            displayText = "0";
                            break;
                        case 2: // 状态1
                            circleColor = wxColour(0, 128, 0);
                            fillColor = wxColour(0, 128, 0);
                            displayText = "1";
                            break;
                        default: // 状态X
                            circleColor = wxColour(0xCC, 0xE0, 0x99);
                            fillColor = wxColour(0xCC, 0xE0, 0x99);
                            displayText = "X";
                            break;
                        }

                        if (arg.fill) {
                            dc.SetBrush(wxBrush(fillColor));
                        }
                        else {
                            dc.SetBrush(*wxTRANSPARENT_BRUSH);
                        }
                        dc.SetPen(wxPen(circleColor, 1));
                        dc.DrawCircle(off(arg.center), arg.radius);
                        return;
                    }
                }
                else if constexpr (std::is_same_v<T, Text>) {
                    wxString displayText;
                    switch (m_outputState) {
                    case 1: displayText = "0"; break;
                    case 2: displayText = "1"; break;
                    default: displayText = "X"; break;
                    }

                    dc.SetTextForeground(*wxWHITE);
                    dc.SetFont(wxFont(arg.fontSize, wxFONTFAMILY_DEFAULT,
                        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
                    dc.DrawText(displayText, off(arg.pos));
                    return;
                }
            }

            if constexpr (std::is_same_v<T, PolyShape>) {
                std::vector<wxPoint> pts;
                for (const auto& p : arg.pts) {
                    pts.push_back(off(p));
                }
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
                dc.SetPen(wxPen(arg.color, 1));
                if (!pts.empty()) {
                    dc.DrawPolygon(static_cast<int>(pts.size()), pts.data());
                }
            }
            else if constexpr (std::is_same_v<T, Line>) {
                dc.SetPen(wxPen(arg.color, 1));
                dc.DrawLine(off(arg.start), off(arg.end));
            }
            else if constexpr (std::is_same_v<T, Circle>) {
                if (arg.fill) {
                    dc.SetBrush(wxBrush(arg.fillColor));
                }
                else {
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);
                }
                dc.SetPen(wxPen(arg.color, 1));
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
                dc.SetPen(wxPen(arg.color, 1));

                double startRad = arg.startAngle * M_PI / 180.0;
                double endRad = arg.endAngle * M_PI / 180.0;

                std::vector<wxPoint> arcPoints;
                int segments = 32; // 增加分段数提高质量
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
                dc.SetPen(wxPen(arg.color, 1));
                auto bezierPoints = CalculateBezier(arg.p0, arg.p1, arg.p2, 32); // 增加分段数

                std::vector<wxPoint> screenPoints;
                for (const auto& wp : bezierPoints) {
                    Point p{ wp.x, wp.y };
                    screenPoints.push_back(off(p));
                }

                if (!screenPoints.empty()) {
                    dc.DrawLines(static_cast<int>(screenPoints.size()), screenPoints.data());
                }
            }
            else if constexpr (std::is_same_v<T, Path>) {
                // 路径回退绘制
                DrawPathFallback(dc, arg, off);
            }
            };

        std::visit(visitor, shape);
    }

    // 绘制输入引脚（蓝色圆点）
    for (const auto& pin : m_inputPins) {
        wxPoint p = off(pin.pos);
        dc.SetPen(wxPen(wxColour(0, 0, 255), 1)); // 蓝色边框
        dc.SetBrush(wxBrush(wxColour(0, 0, 255))); // 蓝色填充
        dc.DrawCircle(p, 3); // 半径为3的圆点
    }

    // 绘制输出引脚（红色圆点）
    for (const auto& pin : m_outputPins) {
        wxPoint p = off(pin.pos);
        dc.SetPen(wxPen(wxColour(255, 0, 0), 1)); // 红色边框
        dc.SetBrush(wxBrush(wxColour(255, 0, 0))); // 红色填充
        dc.DrawCircle(p, 3); // 半径为3的圆点
    }
}

// 路径回退绘制
void CanvasElement::DrawPathFallback(wxGCDC& gcdc, const Path& arg, std::function<wxPoint(const Point&)> off) const
{
    wxGraphicsContext* gc = gcdc.GetGraphicsContext();
    if (!gc) return;

    std::string d = arg.d;
    if (d.find("A 16 28") != std::string::npos) {
        int verticalLineLength = 40;
        int radius = verticalLineLength / 2;
        // 用矢量绘制路径（替代 DC 折线）
        gc->SetPen(wxPen(arg.stroke, arg.strokeWidth));
        gc->StrokeLine(off(Point(10, 12)).x, off(Point(10, 12)).y, off(Point(10, 52)).x, off(Point(10, 52)).y);
        gc->StrokeLine(off(Point(10, 12)).x, off(Point(10, 12)).y, off(Point(10 + radius, 12)).x, off(Point(10 + radius, 12)).y);
        gc->StrokeLine(off(Point(10, 52)).x, off(Point(10, 52)).y, off(Point(10 + radius, 52)).x, off(Point(10 + radius, 52)).y);

        // 矢量绘制弧线
        wxGraphicsPath path = gc->CreatePath();
        path.AddArc(off(Point(30, 32)).x, off(Point(30, 32)).y, radius,
            -90 * M_PI / 180, 90 * M_PI / 180, true);
        gc->StrokePath(path);
    }
}

void CanvasElement::DrawPathFallback(wxDC& dc, const Path& arg, std::function<wxPoint(const Point&)> off) const
{
    std::string d = arg.d;
    if (d.find("A 16 28") != std::string::npos) {
        int verticalLineLength = 40;
        int radius = verticalLineLength / 2;
        // 用 DC 绘制路径（和你之前 DrawFallback 里的逻辑一致）
        dc.SetPen(wxPen(arg.stroke, arg.strokeWidth));
        dc.DrawLine(off(Point(10, 12)), off(Point(10, 52)));
        dc.DrawLine(off(Point(10, 12)), off(Point(10 + radius, 12)));
        dc.DrawLine(off(Point(10, 52)), off(Point(10 + radius, 52)));
        // 绘制弧线（用折线模拟）
        std::vector<wxPoint> arcPoints;
        for (int angle = -90; angle <= 90; angle += 3) {
            double rad = angle * M_PI / 180.0;
            int x = 30 + static_cast<int>(radius * cos(rad));
            int y = 32 + static_cast<int>(radius * sin(rad));
            arcPoints.push_back(off(Point(x, y)));
        }
        if (arcPoints.size() >= 2) {
            dc.DrawLines(static_cast<int>(arcPoints.size()), arcPoints.data());
        }
    }
}

wxRect CanvasElement::GetBounds() const
{
    // GetBounds 方法保持不变
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
                auto pts = CalculateBezier(arg.p0, arg.p1, arg.p2, 32);
                for (const auto& wp : pts) {
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

    for (const auto& pin : m_inputPins) {
        update(pin.pos);
        update(Point(pin.pos.x - 3, pin.pos.y - 3)); // 更新为圆点的边界
        update(Point(pin.pos.x + 3, pin.pos.y + 3));
    }

    for (const auto& pin : m_outputPins) {
        update(pin.pos);
        update(Point(pin.pos.x - 3, pin.pos.y - 3)); // 更新为圆点的边界
        update(Point(pin.pos.x + 3, pin.pos.y + 3));
    }

    return wxRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

void CanvasElement::SetOutputState(int state)
{
    if (state >= 0 && state <= 2) {
        m_outputState = state;
    }
}

// 设置Pin_Output状态的示例代码：
void SetPinOutputState(CanvasElement& pinOutput, int state) {
    pinOutput.SetOutputState(state);
}

// 获取Pin_Output状态的示例代码：
int GetPinOutputState(const CanvasElement& pinOutput) {
    return pinOutput.GetOutputState();
}