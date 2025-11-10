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
EVT_RIGHT_DOWN(CanvasPanel::OnRightDown) // 右键也触发左键抬起事件
EVT_RIGHT_UP(CanvasPanel::OnRightUp)
EVT_MOTION(CanvasPanel::OnMouseMove)
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
EVT_MOUSEWHEEL(CanvasPanel::OnMouseWheel)
wxEND_EVENT_TABLE()

CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE),
    m_offset(0, 0), m_isPanning(false), m_scale(1.0f),
    m_wireMode(WireMode::Idle), m_selectedIndex(-1), m_isDragging(false),
    m_hoverPinIdx(-1), m_hoverCellIdx(-1), m_hoverCellWire(-1) {
    // 快捷工具栏
    m_quickToolBar = new QuickToolBar(this);

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);
    MyLog("CanvasPanel: constructed\n");
}

void CanvasPanel::OnLeftDown(wxMouseEvent& evt) {
    wxPoint rawScreenPos = evt.GetPosition();
    wxPoint rawCanvasPos = ScreenToCanvas(rawScreenPos);

    // 交给工具管理器处理
    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame && mainFrame->GetToolManager()) {
        mainFrame->GetToolManager()->OnCanvasLeftDown(rawCanvasPos);
        if (mainFrame->GetToolManager()->IsEventHandled()) {
            return; // 工具管理器已处理事件
        }
    }

    // 如果工具管理器没有处理，保留原有的导线控制点点击逻辑
    // 这个逻辑现在应该由工具管理器处理，所以这里理论上不会执行
    evt.Skip();
}

void CanvasPanel::OnMouseMove(wxMouseEvent& evt) {
    wxPoint rawScreenPos = evt.GetPosition();
    wxPoint rawCanvasPos = ScreenToCanvas(rawScreenPos);

    // 交给工具管理器处理
    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame && mainFrame->GetToolManager()) {
        mainFrame->GetToolManager()->OnCanvasMouseMove(rawCanvasPos);
        if (mainFrame->GetToolManager()->IsEventHandled()) {
            return; // 工具管理器已处理事件
        }
    }

    evt.Skip();
}

void CanvasPanel::OnLeftUp(wxMouseEvent& evt) {
    // 交给工具管理器处理
    wxPoint screenpos = evt.GetPosition();
	wxPoint canvasPos = ScreenToCanvas(screenpos);
    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame && mainFrame->GetToolManager()) {
        mainFrame->GetToolManager()->OnCanvasLeftUp(canvasPos);
        if (mainFrame->GetToolManager()->IsEventHandled()) {
            return; // 工具管理器已处理事件
        }
    }

    evt.Skip();
}

void CanvasPanel::OnKeyDown(wxKeyEvent& evt) {
    // 交给工具管理器处理
    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame && mainFrame->GetToolManager()) {
        mainFrame->GetToolManager()->OnCanvasKeyDown(evt);
        if (mainFrame->GetToolManager()->IsEventHandled()) {
            return; // 工具管理器已处理事件
        }
    }

    // 原有的删除元件逻辑
    if (evt.GetKeyCode() == WXK_DELETE && m_selectedIndex != -1) {
        // 删除元件前先更新连接的导线
        for (const auto& aw : m_movingWires) {
            if (aw.wireIdx >= m_wires.size()) continue;
            Wire& wire = m_wires[aw.wireIdx];
            const auto& elem = m_elements[m_selectedIndex];
            const auto& pins = aw.isInput ? elem.GetInputPins() : elem.GetOutputPins();
            if (aw.pinIdx >= pins.size()) continue;

            wxPoint newPin = elem.GetPos() + wxPoint(pins[aw.pinIdx].pos.x, pins[aw.pinIdx].pos.y);
            if (aw.ptIdx == 0)
                wire.pts.front().pos = newPin;
            else
                wire.pts.back().pos = newPin;
        }

        m_elements.erase(m_elements.begin() + m_selectedIndex);
        m_selectedIndex = -1;
        Refresh();
    }
    else {
        evt.Skip();
    }
}

void CanvasPanel::OnMouseWheel(wxMouseEvent& evt) {
    // 交给工具管理器处理
    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame && mainFrame->GetToolManager()) {
        mainFrame->GetToolManager()->OnCanvasMouseWheel(evt);
        if (mainFrame->GetToolManager()->IsEventHandled()) {
            return; // 工具管理器已处理事件
        }
    }

    evt.Skip();
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
    //int maxX = static_cast<int>(sz.x / m_scale);  // 转换为画布坐标
    //int maxY = static_cast<int>(sz.y / m_scale);
    int maxX = sz.x;
    int maxY = sz.y;
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
        /*MyLog("DRAW GREEN CELL: wire=%zu cell=%zu  pos=(%d,%d)\n",
            m_hoverCellWire, m_hoverCellIdx,
            m_hoverCellPos.x, m_hoverCellPos.y);*/
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(0, 255, 0), 1));
        dc.DrawCircle(m_hoverCellPos, 3);
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
    Pin standardPin = clone.GetOutputPins()[0];
    wxPoint standardpos = pos - wxPoint(standardPin.pos.x, standardPin.pos.y);
    clone.SetPos(standardpos);
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

    // 添加这些公有方法
    void CanvasPanel::ClearSelection() {
        m_selectedIndex = -1;
        Refresh();
    }

    void CanvasPanel::SetSelectedIndex(int index) {
        if (index >= 0 && index < (int)m_elements.size()) {
            m_selectedIndex = index;
            Refresh();
        }
    }

    int CanvasPanel::HitTestPublic(const wxPoint& pt) {
        return HitTest(pt);
    }

    bool CanvasPanel::IsClickOnEmptyAreaPublic(const wxPoint& canvasPos) {
        return IsClickOnEmptyArea(canvasPos);
    }

    void CanvasPanel::OnRightDown(wxMouseEvent& evt) {
        // 右键按下时也触发左键抬起事件
        wxPoint rawScreenPos = evt.GetPosition();
        wxPoint rawCanvasPos = ScreenToCanvas(rawScreenPos);
        // 交给工具管理器处理
        MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
        if (mainFrame && mainFrame->GetToolManager()) {
            mainFrame->GetToolManager()->OnCanvasRightDown(rawScreenPos);
            if (mainFrame->GetToolManager()->IsEventHandled()) {
                return; // 工具管理器已处理事件
            }
        }
        evt.Skip();
	}

    void CanvasPanel::OnRightUp(wxMouseEvent& evt) {
        // 右键按下时也触发左键抬起事件
        wxPoint rawScreenPos = evt.GetPosition();
        wxPoint rawCanvasPos = ScreenToCanvas(rawScreenPos);
        // 交给工具管理器处理
        MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
        if (mainFrame && mainFrame->GetToolManager()) {
            mainFrame->GetToolManager()->OnCanvasRightUp(rawCanvasPos);
            if (mainFrame->GetToolManager()->IsEventHandled()) {
                return; // 工具管理器已处理事件
            }
        }
        evt.Skip();
    }