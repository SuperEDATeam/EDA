#include "CanvasPanel.h"
#include "MainFrame.h"
#include "Wire.h"
#include <wx/dcbuffer.h>
#include "CanvasElement.h"
#include "my_log.h"
#include <wx/dcgraph.h>  
#include <wx/graphics.h> 


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
void CanvasPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    wxGCDC* gcdc = nullptr;
    wxGraphicsContext* gc = nullptr;

    if (wxGraphicsRenderer::GetDefaultRenderer()) {
        gcdc = new wxGCDC(dc);
        gc = gcdc->GetGraphicsContext();
    }

    if (gc) {
        // 启用高质量抗锯齿
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

        // 应用缩放和偏移（逻辑坐标 -> 设备坐标）
        gc->Scale(m_scale, m_scale);
        gc->Translate(m_offset.x / m_scale, m_offset.y / m_scale);

        // 高DPI适配：获取设备缩放因子
        double dpiScale = GetContentScaleFactor();
        gc->Scale(dpiScale, dpiScale);

        // 1. 绘制网格（逻辑坐标，线宽随缩放自适应）
        const int grid = 20;
        const wxColour gridColor(240, 240, 240);
        gc->SetPen(wxPen(gridColor, 1.0 / m_scale)); // 笔宽在逻辑坐标下调整

        wxSize sz = GetClientSize();
        int maxX = static_cast<int>(sz.x / (m_scale * dpiScale));
        int maxY = static_cast<int>(sz.y / (m_scale * dpiScale));

        for (int x = 0; x < maxX; x += grid) {
            gc->StrokeLine(x, 0, x, maxY);
        }
        for (int y = 0; y < maxY; y += grid) {
            gc->StrokeLine(0, y, maxX, y);
        }

        // 2. 绘制元素（使用矢量绘制）
        for (size_t i = 0; i < m_elements.size(); ++i) {
            m_elements[i].Draw(*gcdc); // 确保元素内部使用gc绘制

            // 选中状态边框（虚线宽度随缩放调整）
            if ((int)i == m_selectedIndex) {
                wxRect b = m_elements[i].GetBounds();
                gc->SetPen(wxPen(*wxRED, 2.0 / m_scale, wxPENSTYLE_DOT));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRectangle(b.x, b.y, b.width, b.height);
            }
        }

        // 3. 绘制导线（矢量线段）
        gc->SetPen(wxPen(*wxBLACK, 1.5 / m_scale)); // 导线宽度自适应
        for (const auto& w : m_wires) w.Draw(*gcdc);
        if (m_wireMode == WireMode::DragNew) m_tempWire.Draw(*gcdc);

        // 4. 悬停引脚高亮（绿色空心圆）
        if (m_hoverPinIdx != -1) {
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(wxColour(0, 255, 0), 1.0 / m_scale));
            gc->DrawEllipse(m_hoverPinPos.x - 3, m_hoverPinPos.y - 3, 6, 6);
        }

        if (m_hoverCellIdx != -1) {
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(wxColour(0, 255, 0), 1.0 / m_scale));
            gc->DrawEllipse(m_hoverCellPos.x - 3, m_hoverCellPos.y - 3, 6, 6);
        }

        delete gcdc; // 释放资源
    }
    else {
        // 如果无法获取 GraphicsContext，回退到原始绘制方法
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

        // 4. 悬停引脚：绿色空心圆
        if (m_hoverPinIdx != -1) {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);              // 不填充 → 空心
            dc.SetPen(wxPen(wxColour(0, 255, 0), 1));       // 绿色边框，线宽 2
            dc.DrawCircle(m_hoverPinPos, 3);                // 半径 3 像素
        }

        if (m_hoverCellIdx != -1) {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(wxPen(wxColour(0, 255, 0), 1));
            dc.DrawCircle(m_hoverCellPos, 3);
        }
    }
}

void CanvasPanel::OnLeftDown(wxMouseEvent& evt)
{
    wxPoint rawScreenPos = evt.GetPosition();
    wxPoint rawCanvasPos = ScreenToCanvas(rawScreenPos);

    /* ===== 1. 优先处理：点击导线绿色小方块 ===== */
    bool snappedEnd = false;
    int  cellWire, cellIdx;
    wxPoint cellPos;
    int newCell = HitHoverCell(rawCanvasPos, &cellWire, &cellIdx, &cellPos);
    if (m_wireMode == WireMode::Idle && newCell != -1)
    {
        m_startCP = { cellPos, CPType::Pin };
        m_tempWire.Clear();
        m_tempWire.AddPoint(m_startCP);
        m_wireMode = WireMode::DragNew;
        CaptureMouse();
        Refresh();
        return;
    }

    /* ===== 2. 拉线起点：吸附到引脚 ===== */
    bool snapped = false;
    wxPoint pos = Snap(rawCanvasPos, &snapped);
    if (m_wireMode == WireMode::Idle && snapped)
    {
        m_startCP = { pos, CPType::Pin };
        m_tempWire.Clear();
        m_tempWire.AddPoint(m_startCP);
        m_wireMode = WireMode::DragNew;
        CaptureMouse();
        Refresh();
        return;
    }

    /* ===== 3. 拉线完成：双端都可能是引脚 ===== */
    if (m_wireMode == WireMode::DragNew)
    {
        bool snappedEnd = false;
        wxPoint endPos = Snap(pos, &snappedEnd);      // 终点
        ControlPoint end{ endPos, snappedEnd ? CPType::Pin : CPType::Free };
        m_tempWire.AddPoint(end);

        /* 正式存入导线列表 */
        m_wires.emplace_back(m_tempWire);
        Wire& newWire = m_wires.back();
        newWire.GenerateCells();          // 生成小方块

        /* 记录两端连接关系（关键） */
        auto recordConnection = [&](const wxPoint& pinPos, size_t ptIdx)
            {
                for (size_t i = 0; i < m_elements.size(); ++i)
                {
                    const auto& elem = m_elements[i];
                    auto test = [&](const auto& pins, bool isIn)
                        {
                            for (size_t p = 0; p < pins.size(); ++p)
                            {
                                wxPoint w = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
                                if (w == pinPos)
                                {
                                    m_movingWires.push_back({ m_wires.size() - 1, ptIdx, isIn, p });
                                    return true;
                                }
                            }
                            return false;
                        };
                    if (test(elem.GetInputPins(), true)) return;
                    test(elem.GetOutputPins(), false);
                }
            };

        /* 线头 */
        if (newWire.pts.front().type == CPType::Pin)
            recordConnection(newWire.pts.front().pos, 0);

        /* 线尾 */
        if (newWire.pts.back().type == CPType::Pin)
            recordConnection(newWire.pts.back().pos, newWire.pts.size() - 1);

        m_wireMode = WireMode::Idle;
        ReleaseMouse();
        Refresh();
        return;
    }

    /* ===== 4. 元件选择/拖动 ===== */
    m_selectedIndex = HitTest(rawCanvasPos);
    if (m_selectedIndex != -1)
    {
        m_isDragging = true;
        m_dragStartPos = rawScreenPos;
        m_elementStartPos = m_elements[m_selectedIndex].GetPos();

        /* 收集该元件所有引脚对应的导线端点 */
        m_movingWires.clear();
        const auto& elem = m_elements[m_selectedIndex];
        auto collect = [&](const auto& pins, bool isIn)
            {
                for (size_t p = 0; p < pins.size(); ++p)
                {
                    wxPoint pinWorld = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
                    for (size_t w = 0; w < m_wires.size(); ++w)
                    {
                        const auto& wire = m_wires[w];
                        if (!wire.pts.empty() && wire.pts.front().type == CPType::Pin &&
                            wire.pts.front().pos == pinWorld)
                            m_movingWires.push_back({ w, 0, isIn, p });

                        if (wire.pts.size() > 1 &&
                            wire.pts.back().type == CPType::Pin &&
                            wire.pts.back().pos == pinWorld)
                            m_movingWires.push_back({ w, wire.pts.size() - 1, isIn, p });
                    }
                }
            };
        collect(elem.GetInputPins(), true);
        collect(elem.GetOutputPins(), false);
        SetFocus();
        Refresh();
        return;
    }

    /* ===== 5. 放置新元件 ===== */
    MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
    if (mf && !mf->GetPendingTool().IsEmpty())
    {
        PlaceElement(mf->GetPendingTool(), pos);
        mf->ClearPendingTool();
        Refresh();
        return;
    }

    /* ===== 6. Ctrl+空白区 = 平移画布 ===== */
    if (evt.ControlDown())
    {
        m_isPanning = true;
        m_panStartPos = rawScreenPos;
        SetCursor(wxCURSOR_HAND);
        CaptureMouse();
        return;
    }

    Refresh();
}

void CanvasPanel::OnMouseMove(wxMouseEvent& evt)
{
    /* ===== 1. 悬停检测（永远最先做） ===== */
    bool isInput = false;
    wxPoint hoverWorld;
    int newHover = HitHoverPin(evt.GetPosition(), &isInput, &hoverWorld);
    if (newHover != m_hoverPinIdx || isInput != m_hoverIsInput)
    {
        m_hoverPinIdx = newHover;
        m_hoverIsInput = isInput;
        m_hoverPinPos = hoverWorld;
        Refresh();                     // 重画绿圆
    }

    int cellWire, cellIdx;
    wxPoint cellPos;
    int newCell = HitHoverCell(evt.GetPosition(), &cellWire, &cellIdx, &cellPos);
    if (newCell != m_hoverCellIdx || cellWire != m_hoverCellWire)
    {
        m_hoverCellWire = cellWire;
        m_hoverCellIdx = cellIdx;
        m_hoverCellPos = cellPos;
        Refresh();
    }

    /* ===== 2. 各模式处理 ===== */
    if (evt.ControlDown() && m_isPanning)          // 平移画布
    {
        wxPoint delta = evt.GetPosition() - m_panStartPos;
        m_offset += delta;
        m_panStartPos = evt.GetPosition();
        Refresh();
        return;
    }

    if (m_wireMode == WireMode::DragNew)           // 导线预览
    {
        wxPoint rawCanvasPos = ScreenToCanvas(evt.GetPosition());
        bool snapped = false;
        m_curSnap = Snap(rawCanvasPos, &snapped);
        m_tempWire.pts = Wire::RouteOrtho(m_startCP.pos, m_curSnap);
        Refresh();
        return;
    }

    if (m_isDragging && m_selectedIndex != -1)     // 元件拖动
    {
        /* 1. 先更新元件坐标（关键！） */
        wxPoint delta = evt.GetPosition() - m_dragStartPos;
        wxPoint newPos = m_elementStartPos + delta;
        m_elements[m_selectedIndex].SetPos(newPos);

        /* 2. 再重算所有相关导线端点 */
        for (const auto& aw : m_movingWires)
        {
            Wire& wire = m_wires[aw.wireIdx];

            /* 计算新引脚世界坐标 */
            const auto& elem = m_elements[m_selectedIndex];
            const auto& pins = aw.isInput ? elem.GetInputPins()
                : elem.GetOutputPins();
            wxPoint pinOffset = wxPoint(pins[aw.pinIdx].pos.x,
                pins[aw.pinIdx].pos.y);
            wxPoint newPinPos = elem.GetPos() + pinOffset;

            /* 更新导线端点 */
            if (aw.ptIdx == 0)
                wire.pts.front().pos = newPinPos;
            else
                wire.pts.back().pos = newPinPos;

            /* 全程横-竖-横重走线 */
            wire.pts = Wire::RouteOrtho(wire.pts.front().pos,
                wire.pts.back().pos);
        }
        Refresh();
        return;
    }
}

//================= 鼠标释放 =================
void CanvasPanel::OnLeftUp(wxMouseEvent& evt)
{
    MyLog("OnLeftUp 入口  m_selectedIndex=%d  size=%zu\n",
        m_selectedIndex, m_elements.size());

    // 索引无效直接退出
    if (m_selectedIndex == -1 || m_selectedIndex >= m_elements.size())
        return;

    // ===== Ctrl+释放 = 结束平移 =====
    if (evt.ControlDown() && m_isPanning) {
        m_isPanning = false;
        ReleaseMouse();
        SetCursor(wxCURSOR_ARROW);
        return;
    }

    m_isDragging = false;

    /* 关键：把导线端点同步到元件最新引脚 */
    for (const auto& aw : m_movingWires)
    {
        MyLog("for 循环：aw.wireIdx=%zu  m_wires.size=%zu\n",
            aw.wireIdx, m_wires.size());
        if (aw.wireIdx >= m_wires.size()) continue;          // 防导线越界

        Wire& wire = m_wires[aw.wireIdx];
        MyLog("即将访问 wire.pts  pts.size=%zu  ptIdx=%d\n",
            wire.pts.size(), aw.ptIdx);
        if (wire.pts.empty()) continue;                      // 防空向量

        const auto& elem = m_elements[m_selectedIndex];
        const auto& pins = aw.isInput ? elem.GetInputPins()
            : elem.GetOutputPins();
        if (aw.pinIdx >= pins.size()) continue;              // 防引脚越界

        wxPoint newPin = elem.GetPos() + wxPoint(pins[aw.pinIdx].pos.x,
            pins[aw.pinIdx].pos.y);
        if (aw.ptIdx == 0)
            wire.pts.front().pos = newPin;
        else
            wire.pts.back().pos = newPin;
    }

    m_selectedIndex = -1;
    m_movingWires.clear();
}

//================= 键盘 =================
void CanvasPanel::OnKeyDown(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_ESCAPE) {
        MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
        if (mf) mf->ClearPendingTool();
    }
    else if (evt.GetKeyCode() == WXK_DELETE && m_selectedIndex != -1) {
        /* ===== 1. 先同步导线端点（跟 OnLeftUp 里一致） ===== */
        for (const auto& aw : m_movingWires)
        {
            if (aw.wireIdx >= m_wires.size()) continue;
            Wire& wire = m_wires[aw.wireIdx];
            const auto& elem = m_elements[m_selectedIndex];
            const auto& pins = aw.isInput ? elem.GetInputPins()
                : elem.GetOutputPins();
            if (aw.pinIdx >= pins.size()) continue;

            wxPoint newPin = elem.GetPos() + wxPoint(pins[aw.pinIdx].pos.x,
                pins[aw.pinIdx].pos.y);
            if (aw.ptIdx == 0)
                wire.pts.front().pos = newPin;
            else
                wire.pts.back().pos = newPin;
        }

        /* ===== 2. 再删元件、清索引 ===== */
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

// 返回悬停到的引脚索引（-1 表示无），同时输出类型和世界坐标
int CanvasPanel::HitHoverPin(const wxPoint& raw, bool* isInput, wxPoint* worldPos)
{
    for (size_t i = 0; i < m_elements.size(); ++i) {
        const auto& elem = m_elements[i];
        // 输入引脚尖端（突出 1 px）
        for (size_t p = 0; p < elem.GetInputPins().size(); ++p) {
            wxPoint tip = elem.GetPos() + wxPoint(elem.GetInputPins()[p].pos.x - 1,
                elem.GetInputPins()[p].pos.y);
            if (abs(raw.x - tip.x) <= 4 && abs(raw.y - tip.y) <= 4) {
                *isInput = true;
                *worldPos = tip;
                return p;
            }
        }
        // 输出引脚尖端（突出 1 px）
        for (size_t p = 0; p < elem.GetOutputPins().size(); ++p) {
            wxPoint tip = elem.GetPos() + wxPoint(elem.GetOutputPins()[p].pos.x + 1,
                elem.GetOutputPins()[p].pos.y);
            if (abs(raw.x - tip.x) <= 4 && abs(raw.y - tip.y) <= 4) {
                *isInput = false;
                *worldPos = tip;
                return p;
            }
        }
    }
    return -1;
}

int CanvasPanel::HitHoverCell(const wxPoint& raw, int* wireIdx, int* cellIdx, wxPoint* cellPos)
{
    /*MyLog("HitHoverCell: (%d,%d)\n", raw.x, raw.y);
    for (size_t w = 0; w < m_wires.size(); ++w) {
        const auto& wire = m_wires[w];
        for (size_t c = 0; c < wire.cells.size(); ++c) {
            MyLog("  wire[%zu] cell[%zu] = (%d,%d)\n",
                w, c, wire.cells[c].x, wire.cells[c].y);
        }
    }*/
    for (size_t w = 0; w < m_wires.size(); ++w) {
        const auto& wire = m_wires[w];
        for (size_t c = 0; c < wire.cells.size(); ++c) {
            if (abs(raw.x - wire.cells[c].x) <= 2 &&
                abs(raw.y - wire.cells[c].y) <= 2) {
                *wireIdx = w;
                *cellIdx = c;
                *cellPos = wire.cells[c];
                return c;
            }
        }
    }
    return -1;

}

// 清理与指定元件关联的导线
void CanvasPanel::ClearElementWires(size_t elemIndex) {
    if (elemIndex >= m_elements.size()) return;

    const auto& elem = m_elements[elemIndex];
    std::vector<size_t> wiresToRemove;

    // 收集所有与该元件引脚相连的导线
    for (size_t w = 0; w < m_wires.size(); ++w) {
        const auto& wire = m_wires[w];
        if (wire.pts.empty()) continue;

        // 检查导线起点是否连接到元件引脚
        if (wire.pts.front().type == CPType::Pin) {
            wxPoint startPos = wire.pts.front().pos;
            for (const auto& pin : elem.GetInputPins()) {
                wxPoint pinPos = elem.GetPos() + wxPoint(pin.pos.x, pin.pos.y);
                if (startPos == pinPos) {
                    wiresToRemove.push_back(w);
                    break;
                }
            }
            for (const auto& pin : elem.GetOutputPins()) {
                wxPoint pinPos = elem.GetPos() + wxPoint(pin.pos.x, pin.pos.y);
                if (startPos == pinPos) {
                    wiresToRemove.push_back(w);
                    break;
                }
            }
        }

        // 检查导线终点是否连接到元件引脚
        if (wire.pts.size() > 1 && wire.pts.back().type == CPType::Pin) {
            wxPoint endPos = wire.pts.back().pos;
            for (const auto& pin : elem.GetInputPins()) {
                wxPoint pinPos = elem.GetPos() + wxPoint(pin.pos.x, pin.pos.y);
                if (endPos == pinPos) {
                    wiresToRemove.push_back(w);
                    break;
                }
            }
            for (const auto& pin : elem.GetOutputPins()) {
                wxPoint pinPos = elem.GetPos() + wxPoint(pin.pos.x, pin.pos.y);
                if (endPos == pinPos) {
                    wiresToRemove.push_back(w);
                    break;
                }
            }
        }
    }

    // 反向删除导线（避免迭代器失效）
    std::sort(wiresToRemove.rbegin(), wiresToRemove.rend());
    for (size_t idx : wiresToRemove) {
        if (idx < m_wires.size()) {
            m_wires.erase(m_wires.begin() + idx);
        }
    }
}

// 删除选中的元件及关联导线
void CanvasPanel::DeleteSelectedElement() {
    if (m_selectedIndex == -1) return; // 无选中元件

    // 1. 清理关联的导线
    ClearElementWires(m_selectedIndex);

    // 2. 删除选中的元件
    m_elements.erase(m_elements.begin() + m_selectedIndex);

    // 3. 重置选中状态
    m_selectedIndex = -1;
    m_isDragging = false;
    m_movingWires.clear();

    // 4. 刷新画布
    Refresh();
    MyLog("CanvasPanel: Element deleted, remaining count: %zu\n", m_elements.size());

}