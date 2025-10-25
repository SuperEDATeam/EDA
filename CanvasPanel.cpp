#include "CanvasPanel.h"
#include "MainFrame.h"
#include "Wire.h"
#include <wx/dcbuffer.h>
#include "CanvasElement.h"
#include "my_log.h"

wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)
EVT_LEFT_UP(CanvasPanel::OnLeftUp)
EVT_MOTION(CanvasPanel::OnMouseMove)
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
EVT_MOUSEWHEEL(CanvasPanel::OnMouseWheel)  // 新增：绑定滚轮事件
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)    // 左键按下（用于开始平移或操作元素）
EVT_LEFT_UP(CanvasPanel::OnLeftUp)        // 左键释放（结束平移或操作元素）
EVT_MOTION(CanvasPanel::OnMouseMove)      // 鼠标移动（处理平移或元素拖拽）
wxEND_EVENT_TABLE()


//================= 构造 =================
CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE),
    m_offset(0, 0),  // 初始化偏移量
    m_isPanning(false)  // 初始化平移状态
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxYELLOW);
    MyLog("CanvasPanel: constructed\n");
}


bool CanvasPanel::IsClickOnEmptyArea(const wxPoint& canvasPos)
{
    // 遍历所有元素，判断点击位置是否在任何元素内部
    for (const auto& elem : m_elements) {
        if (elem.GetBounds().Contains(canvasPos)) {
            return false; // 点击在元素上，不是空白区域
        }
    }
    return true; // 空白区域
}  

// 新增：设置缩放比例（限制范围0.1~5.0，避免过度缩放）
void CanvasPanel::SetScale(float scale)
{
    if (scale < 0.1f) scale = 0.1f;
    if (scale > 5.0f) scale = 5.0f;
    m_scale = scale;
    Refresh();  // 触发重绘
    // 更新状态栏显示缩放比例（如果需要）
    MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
    if (mf) {
        mf->SetStatusText(wxString::Format("Zoom: %.0f%%", m_scale * 100));
    }
}


// 新增：屏幕坐标转画布坐标（除以缩放因子）
wxPoint CanvasPanel::ScreenToCanvas(const wxPoint& screenPos) const
{
    return wxPoint(
        static_cast<int>((screenPos.x - m_offset.x) / m_scale),
        static_cast<int>((screenPos.y - m_offset.y) / m_scale)
    );
}

// 新增：画布坐标转屏幕坐标（乘以缩放因子）
wxPoint CanvasPanel::CanvasToScreen(const wxPoint& canvasPos) const
{
    return wxPoint(
        static_cast<int>(canvasPos.x * m_scale + m_offset.x),
        static_cast<int>(canvasPos.y * m_scale + m_offset.y)
    );
}

//================= 添加元件 =================
void CanvasPanel::AddElement(const CanvasElement& elem)
{
    m_elements.push_back(elem);
    Refresh();
    MyLog("CanvasPanel::AddElement: <%s> total=%zu\n",
        elem.GetName().ToUTF8().data(), m_elements.size());
}

//================= 绘制 =================
// 修改：绘图时应用缩放因子
void CanvasPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    // 应用缩放和偏移
    dc.SetUserScale(m_scale, m_scale);
    dc.SetDeviceOrigin(m_offset.x, m_offset.y);  // 设置设备原点偏移

    // 1. 绘制网格（网格大小随缩放自适应）
    const int grid = 20;
    const wxColour c(240, 240, 240);
    dc.SetPen(wxPen(c, 1));
    // 计算可见区域的网格范围（基于缩放后的画布大小）
    wxSize sz = GetClientSize();
    int maxX = static_cast<int>(sz.x / m_scale);  // 转换为画布坐标
    int maxY = static_cast<int>(sz.y / m_scale);
    for (int x = 0; x < maxX; x += grid)
        dc.DrawLine(x, 0, x, maxY);
    for (int y = 0; y < maxY; y += grid)
        dc.DrawLine(0, y, maxX, y);

    // 2. 绘制元素（元素坐标已在CanvasElement内部维护，缩放由DC自动处理）
    for (size_t i = 0; i < m_elements.size(); ++i) {
        m_elements[i].Draw(dc);
        // 选中状态边框
        if ((int)i == m_selectedIndex) {
            wxRect b = m_elements[i].GetBounds();
            dc.SetPen(wxPen(*wxRED, 2, wxPENSTYLE_DOT));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(b);
        }
    }

    // 3. 绘制导线（导线坐标基于画布，缩放由DC处理）
    for (const auto& w : m_wires) w.Draw(dc);
    if (m_wireMode == WireMode::DragNew) m_tempWire.Draw(dc);
}

void CanvasPanel::OnLeftDown(wxMouseEvent& evt)
{
    wxPoint rawScreenPos = evt.GetPosition();
    wxPoint rawCanvasPos = ScreenToCanvas(rawScreenPos);
    wxPoint canvasPos = rawCanvasPos;

    // 先处理导线绘制模式
    bool snapped = false;
    wxPoint pos = Snap(rawCanvasPos, &snapped);

    if (m_wireMode == WireMode::Idle && snapped) {
        m_startCP = { pos, CPType::Pin };
        m_tempWire.Clear();
        m_tempWire.AddPoint(m_startCP);
        m_wireMode = WireMode::DragNew;
        CaptureMouse();
        Refresh();
        return;
    }
    //if (m_wireMode == WireMode::DragNew) {
    //    ControlPoint end{ pos, snapped ? CPType::Pin : CPType::Free };
    //    m_tempWire.AddPoint(end);

    //    // 关键：只要吸附到引脚，就把两端都标记为 Pin，以便双向收集
    //    if (snapped) {
    //        m_tempWire.pts.front().type = CPType::Pin;
    //        MyLog("Before: back.type=%d\n", int(m_tempWire.pts.back().type));
    //        m_tempWire.pts.back().type = CPType::Pin;
    //        MyLog("After : back.type=%d\n", int(m_tempWire.pts.back().type));
    //    }

    //    MyLog("After set: wire front type=%d  back type=%d\n",
    //        int(m_tempWire.pts.front().type),
    //        int(m_tempWire.pts.back().type));

    //    MyLog("Assign: &front=%p  &back=%p  size=%zu\n",
    //        &m_tempWire.pts.front().type,
    //        &m_tempWire.pts.back().type,
    //        m_tempWire.pts.size());
    //    m_wires.emplace_back(m_tempWire);
    //    m_wireMode = WireMode::Idle;
    //    ReleaseMouse();
    //    Refresh();
    //    return;
    //}
    if (m_wireMode == WireMode::DragNew) {
        bool snappedEnd = false;
        wxPoint endPos = Snap(pos, &snappedEnd);   // 用 Snap 保证落在引脚或网格

        ControlPoint end{ endPos, snappedEnd ? CPType::Pin : CPType::Free };
        m_tempWire.AddPoint(end);

        if (snappedEnd) {
            m_tempWire.pts.front().type = CPType::Pin;
            m_tempWire.pts.back().type = CPType::Pin;
        }

        m_wires.emplace_back(m_tempWire);
        m_wireMode = WireMode::Idle;
        ReleaseMouse();
        Refresh();
        return;
    }

    // 处理元件选择和拖动
    m_selectedIndex = HitTest(rawCanvasPos);
    if (m_selectedIndex != -1) {
        m_isDragging = true;
        m_dragStartPos = rawScreenPos;
        m_elementStartPos = m_elements[m_selectedIndex].GetPos();

        // ===== 双向收集：起点 OR 终点连到被拖元件引脚 =====
        m_movingWires.clear();
        const CanvasElement& elem = m_elements[m_selectedIndex];
        MyLog("Drag elem POS = (%d,%d)\n", elem.GetPos().x, elem.GetPos().y);
        auto collect = [&](const auto& pins, bool isInput) {
            for (size_t p = 0; p < pins.size(); ++p) {
                wxPoint pinWorld = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
                MyLog("  pin[%zu] world = (%d,%d)\n", p, pinWorld.x, pinWorld.y);
                for (size_t w = 0; w < m_wires.size(); ++w) {
                    const auto& wire = m_wires[w];

                    MyLog("    wire[%zu] front=(%d,%d) type=%d  back=(%d,%d) type=%d\n",
                        w,
                        wire.pts.front().pos.x, wire.pts.front().pos.y, int(wire.pts.front().type),
                        wire.pts.back().pos.x, wire.pts.back().pos.y, int(wire.pts.back().type));

                    // 起点命中
                    if (wire.pts.front().type == CPType::Pin &&
                        wire.pts.front().pos == pinWorld) {
                        m_movingWires.push_back({ w, 0, isInput, p });
                        MyLog("      HIT START\n");
                        MyLog("Collect START wire=%zu pt=0 pin=%zu\n", w, p);
                    }
                    // 终点命中
                    if (wire.pts.size() > 1 &&
                        wire.pts.back().type == CPType::Pin &&
                        wire.pts.back().pos == pinWorld) {
                        m_movingWires.push_back({ w, wire.pts.size() - 1, isInput, p });
                        MyLog("      HIT END\n");
                        MyLog("Collect END   wire=%zu pt=%zu pin=%zu\n", w, wire.pts.size() - 1, p);
                    }
                }
            }
            };
        collect(elem.GetInputPins(), true);
        collect(elem.GetOutputPins(), false);
        MyLog("Collect total=%zu\n", m_movingWires.size());
        SetFocus();
        Refresh();
        return;
    }

    // 处理元件放置（优先于平移）
    MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
    if (mf && !mf->GetPendingTool().IsEmpty()) {
        PlaceElement(mf->GetPendingTool(), pos);
        mf->ClearPendingTool();
        Refresh();
        return;
    }

    // 最后处理空白区域的平移
    if (IsClickOnEmptyArea(canvasPos)) {
        m_isPanning = true;
        m_panStartPos = rawScreenPos;
        SetCursor(wxCURSOR_HAND);
        CaptureMouse();
        return;
    }

    Refresh();
}






// 修改：鼠标移动事件中，适配缩放后的坐标
void CanvasPanel::OnMouseMove(wxMouseEvent& evt)
{

    // 优先处理平移
    if (m_isPanning) {
        wxPoint delta = evt.GetPosition() - m_panStartPos; // 计算屏幕坐标偏移
        m_offset += delta; // 更新画布偏移量（平移核心）
        m_panStartPos = evt.GetPosition(); // 更新起点
        Refresh(); // 重绘画布
        return;
    }
    // 平移处理
    if (m_isPanning) {
        wxPoint delta = evt.GetPosition() - m_panStartPos;
        m_offset += delta;
        m_panStartPos = evt.GetPosition();
        Refresh();
        return;
    }
    if (m_wireMode == WireMode::DragNew) {
        // 导线预览：鼠标位置转换为画布坐标
        wxPoint rawCanvasPos = ScreenToCanvas(evt.GetPosition());
        bool snapped = false;
        m_curSnap = Snap(rawCanvasPos, &snapped);
        auto route = Wire::RouteOrtho(m_startCP.pos, m_curSnap);
        m_tempWire.pts = route;
        Refresh();
        return;
    }

    // ---- 元件拖动 + 连线实时重路由（Manhattan） ----
    if (m_isDragging && m_selectedIndex != -1) {
        wxPoint delta = evt.GetPosition() - m_dragStartPos;
        m_elements[m_selectedIndex].SetPos(m_elementStartPos + delta);

        // 对所有连到被拖元件引脚的线段：整条重新生成横-竖-横
        for (const auto& aw : m_movingWires) {
            Wire& wire = m_wires[aw.wireIdx];

            // 重新计算两端坐标
            wxPoint newPin = m_elements[m_selectedIndex].GetPos() +
                wxPoint((aw.isInput ?
                    m_elements[m_selectedIndex].GetInputPins()[aw.pinIdx].pos :
                    m_elements[m_selectedIndex].GetOutputPins()[aw.pinIdx].pos).x,
                    (aw.isInput ?
                        m_elements[m_selectedIndex].GetInputPins()[aw.pinIdx].pos :
                        m_elements[m_selectedIndex].GetOutputPins()[aw.pinIdx].pos).y);

            wxPoint startPt = (aw.ptIdx == 0) ? newPin : wire.pts.front().pos;
            wxPoint endPt = (aw.ptIdx == wire.pts.size() - 1) ? newPin : wire.pts.back().pos;

            // 丢弃旧折点，全程横-竖-横
            wire.pts = Wire::RouteOrtho(startPt, endPt);
        }

        Refresh();
    }
}

//================= 鼠标释放 =================
void CanvasPanel::OnLeftUp(wxMouseEvent& evt)
{
    if (m_isPanning) {
        // 结束平移
        m_isPanning = false;
        ReleaseMouse();
        SetCursor(wxCURSOR_ARROW); // 恢复箭头光标
        return;
    }
    m_isDragging = false;
    m_movingWires.clear();   // 拖动结束即可清空
}

//================= 键盘 =================
void CanvasPanel::OnKeyDown(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_ESCAPE) {
        MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
        if (mf) mf->ClearPendingTool();
    }
    else if (evt.GetKeyCode() == WXK_DELETE && m_selectedIndex != -1) {
        m_elements.erase(m_elements.begin() + m_selectedIndex);
        m_selectedIndex = -1;
        Refresh();
    }
    else {
        evt.Skip();
    }
}

//================= 放置元件 =================
void CanvasPanel::PlaceElement(const wxString& name, const wxPoint& pos)
{
    extern std::vector<CanvasElement> g_elements;
    auto it = std::find_if(g_elements.begin(), g_elements.end(),
        [&](const CanvasElement& e) { return e.GetName() == name; });
    if (it == g_elements.end()) return;
    CanvasElement clone = *it;
    clone.SetPos(pos);
    AddElement(clone);
}
// 修改：HitTest使用画布坐标判断
int CanvasPanel::HitTest(const wxPoint& pt)  // pt已转换为画布坐标
{
    for (size_t i = 0; i < m_elements.size(); ++i) {
        // 元素的边界是画布坐标，直接比较
        if (m_elements[i].GetBounds().Contains(pt)) {
            return i;
        }
    }
    return -1;
}
// 修改鼠标滚轮事件，仅在Ctrl键按下时缩放
void CanvasPanel::OnMouseWheel(wxMouseEvent& evt)
{
    // 只有按住Ctrl键时才处理缩放
    if (!evt.ControlDown()) {
        evt.Skip();
        return;
    }

    // 获取鼠标在画布上的位置（用于缩放中心计算）
    wxPoint mouseScreenPos = evt.GetPosition();
    wxPoint mouseCanvasPos = ScreenToCanvas(mouseScreenPos);

    // 保存当前缩放比例
    float oldScale = m_scale;

    // 根据滚轮方向调整缩放
    if (evt.GetWheelRotation() > 0) {
        SetScale(m_scale * 1.2f);  // 放大
    }
    else {
        SetScale(m_scale / 1.2f);  // 缩小
    }

    // 调整偏移量，使鼠标指向的画布位置保持不变
    wxPoint newMouseScreenPos = CanvasToScreen(mouseCanvasPos);
    m_offset += mouseScreenPos - newMouseScreenPos;

    evt.Skip(false);  // 不再传递事件
}


//================= 吸附：网格+引脚 =================
wxPoint CanvasPanel::Snap(const wxPoint& raw, bool* snapped)
{
    *snapped = false;
    const int grid = 20;
    wxPoint s((raw.x + grid / 2) / grid * grid, (raw.y + grid / 2) / grid * grid);

    // 吸引脚（半径 8 px）
    for (const auto& elem : m_elements) {
        auto testPins = [&](const auto& pins) {
            for (const auto& pin : pins) {
                wxPoint p = elem.GetPos() + wxPoint(pin.pos.x, pin.pos.y);
                if (abs(raw.x - p.x) <= 8 && abs(raw.y - p.y) <= 8) {
                    *snapped = true;
                    return p;
                }
            }
            return wxPoint{};
            };
        wxPoint in = testPins(elem.GetInputPins());
        if (in != wxPoint{}) return in;
        wxPoint out = testPins(elem.GetOutputPins());
        if (out != wxPoint{}) return out;
    }
    return s;
}