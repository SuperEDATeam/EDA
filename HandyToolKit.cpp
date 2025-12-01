// HandyToolKit.cpp
#include "HandyToolKit.h"
#include <wx/dcbuffer.h>
#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/graphics.h>

wxBEGIN_EVENT_TABLE(HandyToolKit, wxPopupWindow)
EVT_PAINT(HandyToolKit::OnPaint)
EVT_MOTION(HandyToolKit::OnMouseMove)
EVT_RIGHT_UP(HandyToolKit::OnRightUp)
EVT_KILL_FOCUS(HandyToolKit::OnKillFocus)
wxEND_EVENT_TABLE()

HandyToolKit::HandyToolKit(CanvasPanel* parent, CanvasEventHandler* ce)
    : wxPopupWindow(parent, wxBORDER_SIMPLE),
	m_canvas(parent),
	m_selectedTool(-1), m_hoveredTool(-1), m_CanvasEventHandler(ce)
{   
    wxInitAllImageHandlers();
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    CreateTools();

    SetExtraStyle(GetExtraStyle() | wxWS_EX_TRANSIENT);

    // 设置合适的大小
    SetSize(wxSize(5*24, 24));
}

void HandyToolKit::CreateTools()
{
    // 创建工具列表
    m_tools.clear();

    
    // 添加工具
    wxBitmap drag("res\\icons\\poke.png", wxBITMAP_TYPE_PNG);
	drag.Rescale(drag, wxSize(24, 24));
    wxBitmap choose("res\\icons\\select.png", wxBITMAP_TYPE_PNG);
    choose.Rescale(choose, wxSize(24, 24));
    wxBitmap text("res\\icons\\text.png", wxBITMAP_TYPE_PNG);
    text.Rescale(text, wxSize(24, 24));
    wxBitmap wire("res\\icons\\wiring.png", wxBITMAP_TYPE_PNG);
    wire.Rescale(wire, wxSize(24, 24));
    wxBitmap eraser("res\\icons\\eraser.png", wxBITMAP_TYPE_PNG);
    eraser.Rescale(eraser, wxSize(24, 24));
    m_tools.push_back({ "Choose", choose, wxRect(24, 0, 24, 24), [this](void) {m_CanvasEventHandler->SetCurrentTool(ToolType::SELECT_TOOL); } });
    m_tools.push_back({ "Drag", drag, wxRect(0, 0, 24, 24), [this](void) {m_CanvasEventHandler->SetCurrentTool(ToolType::DRAG_TOOL); } });
    m_tools.push_back({ "Eraser", eraser, wxRect(48, 0, 24, 24), [this](void) {m_CanvasEventHandler->SetCurrentTool(ToolType::ERASER_TOOL); } });
    m_tools.push_back({ "Text", text, wxRect(72, 0, 24, 24), [this](void) {m_CanvasEventHandler->SetCurrentTool(ToolType::TEXT_TOOL); } });
    m_tools.push_back({ "Wire", wire, wxRect(96, 0, 24, 24), [this](void) {m_CanvasEventHandler->SetCurrentTool(ToolType::WIRE_TOOL); } });
}

void HandyToolKit::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    wxSize sz = GetClientSize();

    // 绘制半透明背景（更现代的外观）
    wxColour bgColor(250, 250, 250, 230);
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(wxPen(wxColour(200, 200, 200), 1));
    dc.DrawRectangle(0, 0, sz.x, sz.y);

    // 绘制工具按钮
    for (size_t i = 0; i < m_tools.size(); i++) {
        const auto& tool = m_tools[i];



        // 绘制按钮基础样式
        DrawToolButton(dc, tool.rect, i == m_hoveredTool);

        // 绘制图标（居中）
        if (tool.icon.IsOk()) {
            int iconX = tool.rect.x + (tool.rect.width - tool.icon.GetWidth()) / 2;
            int iconY = tool.rect.y + (tool.rect.height - tool.icon.GetHeight()) / 2 - 5;
            dc.DrawBitmap(tool.icon, tool.rect.x, tool.rect.y, true);
        }


    }
}

// 绘制工具按钮的基础样式和悬停效果
void HandyToolKit::DrawToolButton(wxDC& dc, const wxRect& rect, bool isHovered)
{
    // 基础按钮样式
    wxColour buttonColor(240, 240, 240);
    wxColour borderColor(180, 180, 180);

    // 绘制按钮背景
    dc.SetBrush(wxBrush(buttonColor));
    dc.SetPen(wxPen(borderColor, 1));
    dc.DrawRectangle(rect);

    // 如果悬停，添加透明效果
    if (isHovered) {
        // 多种悬停效果可选：

        // 效果1：半透明色块
        DrawSemiTransparentOverlay(dc, rect, wxColour(100, 150, 255), 80);

        // 效果2：发光边框
        // DrawGlowBorder(dc, rect, wxColour(100, 150, 255), 3);

        // 效果3：渐变高亮
        // DrawGradientHighlight(dc, rect, wxColour(200, 220, 255), wxColour(100, 150, 255));
    }
}

// 绘制半透明覆盖层
void HandyToolKit::DrawSemiTransparentOverlay(wxDC& dc, const wxRect& rect, const wxColour& color, int alpha)
{
//#if wxUSE_GRAPHICS_CONTEXT
    //// 使用图形上下文（推荐）
    //wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    //if (gc) {
    //    wxColour transparentColor(color.Red(), color.Green(), color.Blue(), alpha);
    //    gc->SetBrush(transparentColor);
    //    gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);
    //    delete gc;
    //}
//#else
    // 回退方案：使用带透明度的颜色
    wxColour transparentColor(color.Red(), color.Green(), color.Blue(), alpha);
    dc.SetBrush(wxBrush(transparentColor));
    dc.SetPen(wxPen(transparentColor));
    dc.DrawRectangle(rect);
//#endif
}

//// 可选：绘制发光边框效果
void HandyToolKit::DrawGlowBorder(wxDC& dc, const wxRect& rect, const wxColour& color, int glowWidth)
{
//#if wxUSE_GRAPHICS_CONTEXT
//    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
//    if (gc) {
//        // 创建渐变画刷
//        wxGraphicsBrush brush = gc->CreateLinearGradientBrush(
//            rect.x, rect.y,
//            rect.x + rect.width, rect.y + rect.height,
//            wxColour(color.Red(), color.Green(), color.Blue(), 50),
//            wxColour(color.Red(), color.Green(), color.Blue(), 150));
//
//        gc->SetBrush(brush);
//        gc->SetPen(*wxTRANSPARENT_PEN);
//
//        // 绘制稍大的矩形作为发光效果
//        wxRect glowRect = rect;
//        glowRect.Inflate(glowWidth);
//        gc->DrawRectangle(glowRect.x, glowRect.y, glowRect.width, glowRect.height);
//
//        delete gc;
    }
//#endif
//}
//
//// 可选：绘制渐变高亮效果
void HandyToolKit::DrawGradientHighlight(wxDC& dc, const wxRect& rect, const wxColour& startColor, const wxColour& endColor)
{
//#if wxUSE_GRAPHICS_CONTEXT
//    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
//    if (gc) {
//        // 创建垂直渐变
//        wxGraphicsBrush brush = gc->CreateLinearGradientBrush(
//            rect.x, rect.y,
//            rect.x, rect.y + rect.height,
//            startColor, endColor);
//
//        gc->SetBrush(brush);
//        gc->SetPen(*wxTRANSPARENT_PEN);
//        gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);
//
//        delete gc;
    }
//#endif
//}
void HandyToolKit::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int oldHovered = m_hoveredTool;
    m_hoveredTool = -1;

    // 检查鼠标在哪个工具上
    for (size_t i = 0; i < m_tools.size(); i++) {
        if (m_tools[i].rect.Contains(pos)) {
			m_canvas->SetStatus(wxString::Format("工具: %s", m_tools[i].name.ToUTF8().data()));
            m_hoveredTool = i;
            break;
        }
    }

    if (m_hoveredTool != oldHovered) {
        Refresh();
    }

    event.Skip();
}

void HandyToolKit::OnRightUp(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();

    // 确定选择了哪个工具
    for (size_t i = 0; i < m_tools.size(); i++) {
        if (m_tools[i].rect.Contains(pos)) {
            m_selectedTool = i;
			// 执行工具的动作
			m_tools[i].action();
            PassFocusToCanvas();
            break;
        }
    }

    // 关闭工具栏
    Hide();

    // 注意：这里不调用event.Skip()，因为我们处理了这个事件
}

//void QuickToolBar::OnKillFocus(wxFocusEvent& event)
//{
//    // 失去焦点时自动关闭
//    Hide();
//    event.Skip();
//}

void HandyToolKit::OnKillFocus(wxFocusEvent& event) {
    wxWindow* focused = wxWindow::FindFocus();

    // 只有当焦点转移到非父窗口时才隐藏
    if (focused && focused != GetParent()) {
        Hide();
        //MyLog("QuickToolBar: Hidden due to focus loss\n");
    }

    event.Skip();
}

void HandyToolKit::PassFocusToCanvas() {
	// 将焦点传递回画布
	m_canvas->SetFocus();
}