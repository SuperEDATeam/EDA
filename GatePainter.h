#pragma once

#include "CanvasElement.h"
#include <vector>
#include <memory>

// 门绘制器接口
class IGatePainter {
public:
    virtual ~IGatePainter() = default;

    // 生成门的形状列表
    virtual std::vector<Shape> GenerateShapes(const GateProperties& props) = 0;

    // 生成输入引脚位置
    virtual std::vector<Pin> GenerateInputPins(const GateProperties& props) = 0;

    // 生成输出引脚位置
    virtual std::vector<Pin> GenerateOutputPins(const GateProperties& props) = 0;

protected:
    // 应用朝向变换
    static Point TransformByFacing(const Point& p, const wxString& facing, const Point& center);

    // 获取默认颜色
    static wxColour GetDefaultColor() { return wxColour(0x33, 0x33, 0x33); }

    // 取反圆圈半径
    static const int NEGATE_CIRCLE_RADIUS = 4;
};

// AND门绘制器
class AndGatePainter : public IGatePainter {
public:
    std::vector<Shape> GenerateShapes(const GateProperties& props) override;
    std::vector<Pin> GenerateInputPins(const GateProperties& props) override;
    std::vector<Pin> GenerateOutputPins(const GateProperties& props) override;

private:
    // 获取AND门尺寸参数（根据gateSize进行缩放）
    static GateSizeParams GetAndGateSizeParams(const wxString& gateSize);

    // 生成AND门基本形状(直线+半圆弧)
    void GenerateAndGateBody(std::vector<Shape>& shapes, const GateProperties& props,
        int width, int height, const Point& center);

    // 生成取反圆圈
    void GenerateNegateCircles(std::vector<Shape>& shapes, const GateProperties& props,
        const std::vector<Pin>& inputPins);

    // 生成标签文本
    void GenerateLabelText(std::vector<Shape>& shapes, const GateProperties& props,
        int width, int height, const Point& center);
};

// OR门绘制器
class OrGatePainter : public IGatePainter {
public:
    std::vector<Shape> GenerateShapes(const GateProperties& props) override;
    std::vector<Pin> GenerateInputPins(const GateProperties& props) override;
    std::vector<Pin> GenerateOutputPins(const GateProperties& props) override;

protected:
    // 生成OR门基本形状(贝塞尔曲线)
    virtual void GenerateOrGateBody(std::vector<Shape>& shapes, const GateProperties& props,
        int width, int height, int outputX, int outputY, const Point& center);

    // 生成取反圆圈
    void GenerateNegateCircles(std::vector<Shape>& shapes, const GateProperties& props,
        const std::vector<Pin>& inputPins);

    // 生成标签文本
    void GenerateLabelText(std::vector<Shape>& shapes, const GateProperties& props,
        int width, int height, const Point& center);
};

// XOR门绘制器(继承自OR门，添加额外曲线)
class XorGatePainter : public OrGatePainter {
protected:
    // 重写生成XOR门形状(OR门形状+额外的左侧曲线)
    void GenerateOrGateBody(std::vector<Shape>& shapes, const GateProperties& props,
        int width, int height, int outputX, int outputY, const Point& center) override;
};

// 门绘制器工厂
class GatePainterFactory {
public:
    static std::unique_ptr<IGatePainter> CreatePainter(const wxString& gateType);
};