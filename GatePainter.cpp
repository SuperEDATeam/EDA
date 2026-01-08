#include "GatePainter.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==================== IGatePainter 基类实现 ====================

Point IGatePainter::TransformByFacing(const Point& p, const wxString& facing, const Point& center) {
    // 相对于中心点的坐标
    int rx = p.x - center.x;
    int ry = p.y - center.y;

    int newX, newY;

    if (facing == "East") {
        // 0度，不变换
        newX = rx;
        newY = ry;
    }
    else if (facing == "South") {
        // 90度顺时针
        newX = -ry;
        newY = rx;
    }
    else if (facing == "West") {
        // 180度
        newX = -rx;
        newY = -ry;
    }
    else if (facing == "North") {
        // 270度顺时针 (或90度逆时针)
        newX = ry;
        newY = -rx;
    }
    else {
        // 默认East
        newX = rx;
        newY = ry;
    }

    return Point(center.x + newX, center.y + newY);
}

// ==================== AndGatePainter 实现 ====================

// 获取AND门尺寸参数（基于middle尺寸进行缩放）
GateSizeParams AndGatePainter::GetAndGateSizeParams(const wxString& gateSize) {
    // middle尺寸（对应json中的样式）
    const int MIDDLE_WIDTH = 80;  // 从左侧线到右侧输出引脚的水平距离
    const int MIDDLE_HEIGHT = 80; // 门主体高度
    const int MIDDLE_PIN_SPACING = 40; // 引脚间距

    double scale = 1.0;

    if (gateSize == "Narrow") {
        scale = 0.5;
    }
    else if (gateSize == "Wide") {
        scale = 2.0;
    }
    // Middle保持1.0

    int width = static_cast<int>(MIDDLE_WIDTH * scale);
    int height = static_cast<int>(MIDDLE_HEIGHT * scale);
    int pinSpacing = static_cast<int>(MIDDLE_PIN_SPACING * scale);

    return GateSizeParams(width, height, pinSpacing);
}

std::vector<Shape> AndGatePainter::GenerateShapes(const GateProperties& props) {
    std::vector<Shape> shapes;

    // 使用缩放后的尺寸参数
    GateSizeParams sizeParams = GetAndGateSizeParams(props.gateSize);
    int width = sizeParams.width;
    int height = sizeParams.height;

    // 中心点固定为 (80, 60)
    Point center(80, 60);

    // 生成AND门主体
    GenerateAndGateBody(shapes, props, width, height, center);

    // 生成取反圆圈
    std::vector<Pin> inputPins = GenerateInputPins(props);
    GenerateNegateCircles(shapes, props, inputPins);

    // 生成标签
    GenerateLabelText(shapes, props, width, height, center);

    return shapes;
}

void AndGatePainter::GenerateAndGateBody(std::vector<Shape>& shapes, const GateProperties& props,
    int width, int height, const Point& center) {
    wxColour color = GetDefaultColor();

    // 计算缩放因子（基于middle尺寸）
    double scale = 1.0;
    if (props.gateSize == "Narrow") {
        scale = 0.5;
    }
    else if (props.gateSize == "Wide") {
        scale = 2.0;
    }

    // 原始middle尺寸的布局参数
    const int MID_LEFT_MARGIN = 40;
    const int MID_BASE_HEIGHT = 80;
    const int MID_TOP_MARGIN = 20;

    // 根据缩放因子计算实际尺寸
    int leftMargin = static_cast<int>(MID_LEFT_MARGIN * scale);
    int topMargin = static_cast<int>(MID_TOP_MARGIN * scale);
    int baseHeight = static_cast<int>(MID_BASE_HEIGHT * scale);
    int arcRadius = baseHeight / 2;

    int left = leftMargin;
    // 门主体的上下边界
    int gateTop = topMargin;
    int gateBottom = topMargin + baseHeight;
    int arcCenterX = leftMargin + width - arcRadius;
    int arcCenterY = topMargin + baseHeight / 2;  // 弧线在门主体中央

    // 计算引脚需要的总高度
    int pinSpacing = GetAndGateSizeParams(props.gateSize).pinSpacing;
    int totalPinHeight = (props.numberOfInputs - 1) * pinSpacing;

    // 左侧线的实际上下边界（可能超出门主体）
    int lineTop, lineBottom;
    if (totalPinHeight <= baseHeight) {
        // 引脚在门主体内，左侧线与门主体一样高
        lineTop = gateTop;
        lineBottom = gateBottom;
    }
    else {
        // 引脚超出门主体，左侧线需要延长
        int extraHeight = (totalPinHeight - baseHeight) / 2;
        lineTop = gateTop - extraHeight;
        lineBottom = gateBottom + extraHeight;
    }

    // 左侧垂直线（可能延长）
    Point p1(left, lineTop);
    Point p2(left, lineBottom);
    p1 = TransformByFacing(p1, props.facing, center);
    p2 = TransformByFacing(p2, props.facing, center);
    shapes.push_back(Line(p1, p2, color));

    // 上边水平线（门主体）
    Point p3(left, gateTop);
    Point p4(arcCenterX, gateTop);
    p3 = TransformByFacing(p3, props.facing, center);
    p4 = TransformByFacing(p4, props.facing, center);
    shapes.push_back(Line(p3, p4, color));

    // 下边水平线（门主体）
    Point p5(left, gateBottom);
    Point p6(arcCenterX, gateBottom);
    p5 = TransformByFacing(p5, props.facing, center);
    p6 = TransformByFacing(p6, props.facing, center);
    shapes.push_back(Line(p5, p6, color));

    // 右侧连接线（从上边到弧线顶部）
    Point pt1(arcCenterX, gateTop);
    Point pt2(arcCenterX, arcCenterY - arcRadius);
    pt1 = TransformByFacing(pt1, props.facing, center);
    pt2 = TransformByFacing(pt2, props.facing, center);
    // 只有当上边和弧线顶部不重合时才画
    if (gateTop < arcCenterY - arcRadius) {
        shapes.push_back(Line(pt1, pt2, color));
    }

    // 右侧连接线（从弧线底部到下边）
    Point pb1(arcCenterX, arcCenterY + arcRadius);
    Point pb2(arcCenterX, gateBottom);
    pb1 = TransformByFacing(pb1, props.facing, center);
    pb2 = TransformByFacing(pb2, props.facing, center);
    // 只有当弧线底部和下边不重合时才画
    if (arcCenterY + arcRadius < gateBottom) {
        shapes.push_back(Line(pb1, pb2, color));
    }

    // 右侧半圆弧
    Point arcCenter(arcCenterX, arcCenterY);
    arcCenter = TransformByFacing(arcCenter, props.facing, center);

    double startAngle = -90;
    double endAngle = 90;
    if (props.facing == "South") {
        startAngle = 0;
        endAngle = 180;
    }
    else if (props.facing == "West") {
        startAngle = 90;
        endAngle = 270;
    }
    else if (props.facing == "North") {
        startAngle = 180;
        endAngle = 360;
    }

    shapes.push_back(ArcShape(arcCenter, arcRadius, startAngle, endAngle, color));
}

void AndGatePainter::GenerateNegateCircles(std::vector<Shape>& shapes, const GateProperties& props,
    const std::vector<Pin>& inputPins) {
    wxColour color = GetDefaultColor();

    for (int i = 0; i < props.numberOfInputs && i < (int)props.negateInputs.size(); i++) {
        if (props.negateInputs[i] && i < (int)inputPins.size()) {
            Point circleCenter = inputPins[i].pos;
            if (props.facing == "East") {
                circleCenter.x -= NEGATE_CIRCLE_RADIUS + 2;
            }
            else if (props.facing == "West") {
                circleCenter.x += NEGATE_CIRCLE_RADIUS + 2;
            }
            else if (props.facing == "South") {
                circleCenter.y -= NEGATE_CIRCLE_RADIUS + 2;
            }
            else if (props.facing == "North") {
                circleCenter.y += NEGATE_CIRCLE_RADIUS + 2;
            }

            shapes.push_back(Circle(circleCenter, NEGATE_CIRCLE_RADIUS, color, false));
        }
    }
}

void AndGatePainter::GenerateLabelText(std::vector<Shape>& shapes, const GateProperties& props,
    int width, int height, const Point& center) {
    if (props.label.IsEmpty()) return;

    int fontSize = 12;
    wxString fontStr = props.labelFont;
    int lastSpace = fontStr.Find(' ', true);
    if (lastSpace != wxNOT_FOUND) {
        long size;
        if (fontStr.Mid(lastSpace + 1).ToLong(&size)) {
            fontSize = (int)size;
        }
    }

    // 计算缩放因子
    double scale = 1.0;
    if (props.gateSize == "Narrow") {
        scale = 0.5;
    }
    else if (props.gateSize == "Wide") {
        scale = 2.0;
    }

    // 原始middle尺寸的布局参数
    const int MID_LEFT_MARGIN = 40;
    const int MID_BASE_HEIGHT = 80;
    const int MID_TOP_MARGIN = 20;

    // 根据缩放因子计算实际尺寸
    int leftMargin = static_cast<int>(MID_LEFT_MARGIN * scale);
    int topMargin = static_cast<int>(MID_TOP_MARGIN * scale);
    int baseHeight = static_cast<int>(MID_BASE_HEIGHT * scale);

    // 计算文字宽度（粗略估计）
    int textWidth = props.label.Length() * fontSize / 2;
    int textHeight = fontSize;

    // 门内部中央位置
    int centerX = leftMargin + width / 2 - textWidth / 2;
    int centerY = topMargin + baseHeight / 2 - textHeight / 2;

    Point labelPos(centerX, centerY);
    labelPos = TransformByFacing(labelPos, props.facing, center);

    shapes.push_back(Text(labelPos, props.label, fontSize, GetDefaultColor()));
}

std::vector<Pin> AndGatePainter::GenerateInputPins(const GateProperties& props) {
    std::vector<Pin> pins;

    GateSizeParams sizeParams = GetAndGateSizeParams(props.gateSize);
    int width = sizeParams.width;
    int baseHeight = sizeParams.height;
    int pinSpacing = sizeParams.pinSpacing;

    // 计算缩放因子
    double scale = 1.0;
    if (props.gateSize == "Narrow") {
        scale = 0.5;
    }
    else if (props.gateSize == "Wide") {
        scale = 2.0;
    }

    // 原始middle尺寸的布局参数
    const int MID_LEFT_MARGIN = 40;
    const int MID_TOP_MARGIN = 20;

    // 根据缩放因子计算实际尺寸
    int leftMargin = static_cast<int>(MID_LEFT_MARGIN * scale);
    int topMargin = static_cast<int>(MID_TOP_MARGIN * scale);

    Point center(80, 60);

    int totalPinHeight = (props.numberOfInputs - 1) * pinSpacing;

    // 引脚垂直居中于门主体中心
    int gateCenterY = topMargin + baseHeight / 2;
    int startY = gateCenterY - totalPinHeight / 2;

    for (int i = 0; i < props.numberOfInputs; i++) {
        Point pinPos(leftMargin, startY + i * pinSpacing);
        pinPos = TransformByFacing(pinPos, props.facing, center);

        wxString pinName = wxString::Format("Input %d", i + 1);
        pins.push_back(Pin(pinPos, pinName, true));
    }

    return pins;
}

std::vector<Pin> AndGatePainter::GenerateOutputPins(const GateProperties& props) {
    std::vector<Pin> pins;

    GateSizeParams sizeParams = GetAndGateSizeParams(props.gateSize);
    int width = sizeParams.width;
    int baseHeight = sizeParams.height;

    // 计算缩放因子
    double scale = 1.0;
    if (props.gateSize == "Narrow") {
        scale = 0.5;
    }
    else if (props.gateSize == "Wide") {
        scale = 2.0;
    }

    // 原始middle尺寸的布局参数
    const int MID_LEFT_MARGIN = 40;
    const int MID_TOP_MARGIN = 20;

    // 根据缩放因子计算实际尺寸
    int leftMargin = static_cast<int>(MID_LEFT_MARGIN * scale);
    int topMargin = static_cast<int>(MID_TOP_MARGIN * scale);

    Point center(80, 60);

    // 输出引脚在门主体的右侧中央
    Point pinPos(leftMargin + width, topMargin + baseHeight / 2);
    pinPos = TransformByFacing(pinPos, props.facing, center);

    pins.push_back(Pin(pinPos, "Output", false));

    return pins;
}

// ==================== OrGatePainter 实现 ====================

static GateSizeParams GetOrGateSizeParams(const wxString& gateSize) {
    if (gateSize == "Narrow") {
        return GateSizeParams(35, 24, 22);
    }
    else if (gateSize == "Wide") {
        return GateSizeParams(55, 36, 34);
    }
    else {
        return GateSizeParams(45, 30, 28);
    }
}

std::vector<Shape> OrGatePainter::GenerateShapes(const GateProperties& props) {
    std::vector<Shape> shapes;

    GateSizeParams sizeParams = GetOrGateSizeParams(props.gateSize);
    int width = sizeParams.width;
    int height = sizeParams.height;

    // 输出引脚位置（曲线要连接到这里）- 与 GenerateOutputPins 保持一致
    int outputX = width;  // 输出引脚在门右侧尖端
    int outputY = height / 2;

    Point center(width / 2, height / 2);

    GenerateOrGateBody(shapes, props, width, height, outputX, outputY, center);

    std::vector<Pin> inputPins = GenerateInputPins(props);
    GenerateNegateCircles(shapes, props, inputPins);

    GenerateLabelText(shapes, props, width, height, center);

    return shapes;
}

void OrGatePainter::GenerateOrGateBody(std::vector<Shape>& shapes, const GateProperties& props,
    int width, int height, int outputX, int outputY, const Point& center) {
    wxColour color = GetDefaultColor();

    // OR 门形状：
    // - 左侧内凹曲线（输入引脚在这条曲线上）
    // - 上边外凸曲线：从左上角直接连接到输出引脚位置（红色点）
    // - 下边外凸曲线：从左下角直接连接到输出引脚位置（红色点）

    int left = 0;
    int top = 0;
    int bottom = height;
    int midY = height / 2;

    // 左侧内凹的深度
    int backCurveDepth = width / 5;

    // 左侧内凹曲线（从上到下）
    Point pLeft0(left, top);
    Point pLeft1(left + backCurveDepth, midY);  // 控制点在中间偏右
    Point pLeft2(left, bottom);
    pLeft0 = TransformByFacing(pLeft0, props.facing, center);
    pLeft1 = TransformByFacing(pLeft1, props.facing, center);
    pLeft2 = TransformByFacing(pLeft2, props.facing, center);
    shapes.push_back(BezierShape(pLeft0, pLeft1, pLeft2, color));

    // 上边曲线（从左上角直接连接到输出引脚位置）
    Point pTop0(left, top);
    Point pTop1(width * 2 / 3, top - height / 6);  // 控制点向上凸出
    Point pTop2(outputX, outputY);  // 直接连接到输出引脚位置
    pTop0 = TransformByFacing(pTop0, props.facing, center);
    pTop1 = TransformByFacing(pTop1, props.facing, center);
    pTop2 = TransformByFacing(pTop2, props.facing, center);
    shapes.push_back(BezierShape(pTop0, pTop1, pTop2, color));

    // 下边曲线（从左下角直接连接到输出引脚位置）
    Point pBot0(left, bottom);
    Point pBot1(width * 2 / 3, bottom + height / 6);  // 控制点向下凸出
    Point pBot2(outputX, outputY);  // 直接连接到输出引脚位置
    pBot0 = TransformByFacing(pBot0, props.facing, center);
    pBot1 = TransformByFacing(pBot1, props.facing, center);
    pBot2 = TransformByFacing(pBot2, props.facing, center);
    shapes.push_back(BezierShape(pBot0, pBot1, pBot2, color));
}

void OrGatePainter::GenerateNegateCircles(std::vector<Shape>& shapes, const GateProperties& props,
    const std::vector<Pin>& inputPins) {
    wxColour color = GetDefaultColor();

    for (int i = 0; i < props.numberOfInputs && i < (int)props.negateInputs.size(); i++) {
        if (props.negateInputs[i] && i < (int)inputPins.size()) {
            Point circleCenter = inputPins[i].pos;
            if (props.facing == "East") {
                circleCenter.x -= NEGATE_CIRCLE_RADIUS + 2;
            }
            else if (props.facing == "West") {
                circleCenter.x += NEGATE_CIRCLE_RADIUS + 2;
            }
            else if (props.facing == "South") {
                circleCenter.y -= NEGATE_CIRCLE_RADIUS + 2;
            }
            else if (props.facing == "North") {
                circleCenter.y += NEGATE_CIRCLE_RADIUS + 2;
            }

            shapes.push_back(Circle(circleCenter, NEGATE_CIRCLE_RADIUS, color, false));
        }
    }
}

void OrGatePainter::GenerateLabelText(std::vector<Shape>& shapes, const GateProperties& props,
    int width, int height, const Point& center) {
    if (props.label.IsEmpty()) return;

    int fontSize = 12;
    wxString fontStr = props.labelFont;
    int lastSpace = fontStr.Find(' ', true);
    if (lastSpace != wxNOT_FOUND) {
        long size;
        if (fontStr.Mid(lastSpace + 1).ToLong(&size)) {
            fontSize = (int)size;
        }
    }

    Point labelPos(width / 2 - props.label.Length() * fontSize / 4, -fontSize - 5);
    labelPos = TransformByFacing(labelPos, props.facing, center);

    shapes.push_back(Text(labelPos, props.label, fontSize, GetDefaultColor()));
}

std::vector<Pin> OrGatePainter::GenerateInputPins(const GateProperties& props) {
    std::vector<Pin> pins;

    GateSizeParams sizeParams = GetOrGateSizeParams(props.gateSize);
    int width = sizeParams.width;
    int height = sizeParams.height;
    int pinSpacing = sizeParams.pinSpacing;

    int backCurveDepth = width / 5;

    int totalPinHeight = (props.numberOfInputs - 1) * pinSpacing;
    int midY = height / 2;
    int startY = midY - totalPinHeight / 2;

    Point center(width / 2, height / 2);

    for (int i = 0; i < props.numberOfInputs; i++) {
        int pinY = startY + i * pinSpacing;
        // 计算左侧曲线上的 X 位置（曲线是内凹的）
        // 使用二次贝塞尔曲线公式计算
        double t = (double)(pinY) / height;
        // 曲线从 (0, 0) 经过控制点 (backCurveDepth, midY) 到 (0, height)
        double pinX = 2 * (1 - t) * t * backCurveDepth;

        Point pinPos((int)pinX, pinY);
        pinPos = TransformByFacing(pinPos, props.facing, center);

        wxString pinName = wxString::Format("Input %d", i + 1);
        pins.push_back(Pin(pinPos, pinName, true));
    }

    return pins;
}

std::vector<Pin> OrGatePainter::GenerateOutputPins(const GateProperties& props) {
    std::vector<Pin> pins;

    GateSizeParams sizeParams = GetOrGateSizeParams(props.gateSize);
    int width = sizeParams.width;
    int height = sizeParams.height;

    Point center(width / 2, height / 2);

    // 输出引脚在右侧尖端
    Point pinPos(width, height / 2);
    pinPos = TransformByFacing(pinPos, props.facing, center);

    pins.push_back(Pin(pinPos, "Output", false));

    return pins;
}

// ==================== XorGatePainter 实现 ====================

void XorGatePainter::GenerateOrGateBody(std::vector<Shape>& shapes, const GateProperties& props,
    int width, int height, int outputX, int outputY, const Point& center) {
    wxColour color = GetDefaultColor();

    int top = 0;
    int bottom = height;
    int midY = height / 2;

    int backDepth = width / 6;
    int topOffset = -height / 6;
    int bottomOffset = height / 6;

    // 额外的左侧曲线（XOR特有）
    Point pe0(-5, top + topOffset);
    Point pe1(backDepth - 5, midY);
    Point pe2(-5, bottom + bottomOffset);
    pe0 = TransformByFacing(pe0, props.facing, center);
    pe1 = TransformByFacing(pe1, props.facing, center);
    pe2 = TransformByFacing(pe2, props.facing, center);
    shapes.push_back(BezierShape(pe0, pe1, pe2, color));

    // 左侧内凹曲线
    Point p0(0, top + topOffset);
    Point p1(backDepth, midY);
    Point p2(0, bottom + bottomOffset);
    p0 = TransformByFacing(p0, props.facing, center);
    p1 = TransformByFacing(p1, props.facing, center);
    p2 = TransformByFacing(p2, props.facing, center);
    shapes.push_back(BezierShape(p0, p1, p2, color));

    // 上边曲线（直接连接到输出引脚位置）
    Point p3(0, top + topOffset);
    Point p4(width / 2, top + topOffset - height / 3);
    Point p5(outputX, outputY);
    p3 = TransformByFacing(p3, props.facing, center);
    p4 = TransformByFacing(p4, props.facing, center);
    p5 = TransformByFacing(p5, props.facing, center);
    shapes.push_back(BezierShape(p3, p4, p5, color));

    // 下边曲线（直接连接到输出引脚位置）
    Point p6(0, bottom + bottomOffset);
    Point p7(width / 2, bottom + bottomOffset + height / 3);
    Point p8(outputX, outputY);
    p6 = TransformByFacing(p6, props.facing, center);
    p7 = TransformByFacing(p7, props.facing, center);
    p8 = TransformByFacing(p8, props.facing, center);
    shapes.push_back(BezierShape(p6, p7, p8, color));
}

// ==================== GatePainterFactory 实现 ====================

std::unique_ptr<IGatePainter> GatePainterFactory::CreatePainter(const wxString& gateType) {
    if (gateType == "AND_Gate" || gateType == "AND_Gate_Rect" ||
        gateType == "NAND_Gate" || gateType == "NAND_Gate_Rect") {
        return std::make_unique<AndGatePainter>();
    }
    else if (gateType == "OR_Gate" || gateType == "OR_Gate_Rect" ||
        gateType == "NOR_Gate" || gateType == "NOR_Gate_Rect") {
        return std::make_unique<OrGatePainter>();
    }
    else if (gateType == "XOR_Gate" || gateType == "XOR_Gate_Rect" ||
        gateType == "XNOR_Gate" || gateType == "XNOR_Gate_Rect") {
        return std::make_unique<XorGatePainter>();
    }

    return std::make_unique<AndGatePainter>();
}