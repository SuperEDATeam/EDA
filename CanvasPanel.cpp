#include "CanvasPanel.h"
#include "MainFrame.h"
#include "Wire.h"
#include <wx/dcbuffer.h>
#include "CanvasElement.h"
#include "my_log.h"
#include <wx/dcgraph.h>  
#include <wx/graphics.h> 
#include "CanvasTextElement.h"


wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)
EVT_LEFT_UP(CanvasPanel::OnLeftUp)
EVT_LEFT_DCLICK(CanvasPanel::OnLeftDoubleClick)  // 添加双击事件
EVT_RIGHT_DOWN(CanvasPanel::OnRightDown) // 右键也触发左键抬起事件
EVT_RIGHT_UP(CanvasPanel::OnRightUp)
EVT_MOTION(CanvasPanel::OnMouseMove)
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
EVT_MOUSEWHEEL(CanvasPanel::OnMouseWheel)
EVT_SET_FOCUS(CanvasPanel::OnFocus)   
EVT_KILL_FOCUS(CanvasPanel::OnKillFocus)
EVT_TIMER(wxID_ANY, CanvasPanel::OnClickTimer)  // 添加定时器事件
wxEND_EVENT_TABLE()

CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE | wxWANTS_CHARS | wxTAB_TRAVERSAL),
    m_offset(0, 0), m_isPanning(false), m_scale(1.0f),
    m_wireMode(WireMode::Idle), m_selectedIndex(-1), m_isDragging(false),
    m_hoverPinIdx(-1), m_hoverCellIdx(-1), m_hoverCellWire(-1), m_hasFocus(false),
    m_hiddenTextCtrl(nullptr), 
    m_isUsingHiddenCtrl(false), m_currentEditingTextIndex(-1) {

    SetupHiddenTextCtrl();


    m_hoverPinIdx(-1), m_hoverCellIdx(-1), m_hoverCellWire(-1),
    m_clickTimer(this)  // 初始化定时器
{
    // 快捷工具栏
    m_quickToolBar = new QuickToolBar(this);
    m_cursorTimer.Start(100);

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);

    SetFocus();
    MyLog("CanvasPanel: constructed\n");
}

//void CanvasPanel::OnLeftDown(wxMouseEvent& evt)
//{
//    wxPoint rawCanvasPos = ScreenToCanvas(evt.GetPosition());
//
//    // 1. 先判断点击位置，无论ToolManager是否处理，CanvasPanel都需要记录状态
//    int idx = HitTest(rawCanvasPos);
//    if (idx != -1) {
//        m_selectedIndex = idx;
//        m_dragStartElemPos = m_elements[idx].GetPos(); // 记录元件起始位置
//        CollectUndoAnchor(idx, m_undoAnchors);         // 记录导线锚点
//        m_isDragging = true;                           // 关键：必须在这里设置标志
//    }
//
//    // 2. 再交给ToolManager处理（它会实际移动元件和导线）
//    MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
//    if (mf && mf->GetToolManager()) {
//        mf->GetToolManager()->OnCanvasLeftDown(rawCanvasPos);
//        // 注意：不再根据IsEventHandled()提前返回
//    }
//
//    // 3. 如果没有点击元件且ToolManager没处理，清除选择
//    if (idx == -1 && mf->GetToolManager() && !mf->GetToolManager()->IsEventHandled()) {
//        ClearSelection();
//    }
//
//    evt.Skip();
//}
void CanvasPanel::OnLeftDown(wxMouseEvent& evt)
{
    wxPoint rawCanvasPos = ScreenToCanvas(evt.GetPosition());

    // 1. 先判断点击位置
    int idx = HitTest(rawCanvasPos);

    // 记录点击信息用于区分单击和双击
    m_clickPos = rawCanvasPos;
    m_clickElementIndex = idx;

    // 如果是Pin_Input元件，启动定时器等待可能的双击
    if (idx != -1 && m_elements[idx].GetId() == "Pin_Input") {
        m_clickTimer.Start(250, true); // 250ms后触发，单次定时器
    }
    else {
        // 对于其他元件，正常处理单击
        if (idx != -1) {
            m_selectedIndex = idx;
            m_dragStartElemPos = m_elements[idx].GetPos();
            CollectUndoAnchor(idx, m_undoAnchors);
            m_isDragging = true;
        }
    }
    EnsureFocus();

    // 交给工具管理器处理
    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame && mainFrame->GetToolManager()) {
        mainFrame->GetToolManager()->OnCanvasLeftDown(rawCanvasPos);
        if (mainFrame->GetToolManager()->IsEventHandled()) {
            return; // 工具管理器已处理事件
        }
    }

    // 2. 保存操作前的导线数量
    m_wireCountBeforeOperation = m_wires.size();

    // 3. 交给ToolManager处理
    MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
    if (mf && mf->GetToolManager()) {
        mf->GetToolManager()->OnCanvasLeftDown(rawCanvasPos);
    }

    // 4. 如果没有点击元件且ToolManager没处理，清除选择
    if (idx == -1 && mf->GetToolManager() && !mf->GetToolManager()->IsEventHandled()) {
        ClearSelection();
    }

    evt.Skip();
}

void CanvasPanel::OnClickTimer(wxTimerEvent& evt)
{
    // 定时器触发，说明没有发生双击，执行单击逻辑
    if (m_clickElementIndex != -1 && m_clickElementIndex < m_elements.size()) {
        // 对于Pin_Input元件，单击时正常选择（不切换状态）
        if (m_elements[m_clickElementIndex].GetId() == "Pin_Input") {
            m_selectedIndex = m_clickElementIndex;
            m_dragStartElemPos = m_elements[m_clickElementIndex].GetPos();
            CollectUndoAnchor(m_clickElementIndex, m_undoAnchors);
            m_isDragging = true;
            Refresh();
        }
    }

    // 重置点击状态
    m_clickElementIndex = -1;
}

void CanvasPanel::OnLeftDoubleClick(wxMouseEvent& evt)
{
    wxPoint rawCanvasPos = ScreenToCanvas(evt.GetPosition());

    // 停止定时器，避免单击逻辑执行
    if (m_clickTimer.IsRunning()) {
        m_clickTimer.Stop();
    }

    // 检查是否双击了Pin_Input或Pin_Output元件
    int idx = HitTest(rawCanvasPos);
    if (idx != -1 && idx < m_elements.size()) {
        wxString elementId = m_elements[idx].GetId();

        if (elementId == "Pin_Input") {
            // 切换Pin_Input的状态
            m_elements[idx].ToggleState();
            Refresh();
            m_isDragging = false;
            evt.Skip(false);
            return;
        }
        else if (elementId == "Pin_Output") {
            // 切换Pin_Output的状态（循环切换：X->0->1->X）
            int currentState = m_elements[idx].GetOutputState();
            int newState = (currentState + 1) % 3; // 0->1->2->0
            m_elements[idx].SetOutputState(newState);
            Refresh();
            m_isDragging = false;
            evt.Skip(false);
            return;
        }
    }

    // 对于其他元件，继续正常处理
    evt.Skip();
}

void CanvasPanel::OnMouseMove(wxMouseEvent& evt) {
    // 如果定时器在运行，说明可能是Pin_Input的单击等待双击，暂时不处理拖动
    if (m_clickTimer.IsRunning()) {
        evt.Skip();
        return;
    }

    wxPoint rawScreenPos = evt.GetPosition();
    wxPoint rawCanvasPos = ScreenToCanvas(rawScreenPos);

    // 交给工具管理器处理
    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame && mainFrame->GetToolManager()) {
        mainFrame->GetToolManager()->OnCanvasMouseMove(rawCanvasPos);
        if (mainFrame->GetToolManager()->IsEventHandled()) {
            return;
        }
    }

    evt.Skip();
}

//void CanvasPanel::OnLeftUp(wxMouseEvent& evt) {
//    wxPoint screenpos = evt.GetPosition();
//    wxPoint canvasPos = ScreenToCanvas(screenpos);
//
//    // 在ToolManager处理前保存拖动状态（关键）
//    bool wasDragging = m_isDragging;
//    int draggedIndex = m_selectedIndex;
//    wxPoint startPos = m_dragStartElemPos;
//    auto anchors = m_undoAnchors; // 复制锚点信息，避免ToolManager清除后丢失
//
//    wxLogDebug("OnLeftUp: wasDragging=%d, idx=%d", wasDragging, draggedIndex); // 调试用
//
//    // 交给ToolManager处理（会实际更新元件和导线位置）
//    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
//    if (mainFrame && mainFrame->GetToolManager()) {
//        mainFrame->GetToolManager()->OnCanvasLeftUp(canvasPos);
//    }
//
//    // 根据CanvasPanel记录的拖动状态生成撤销命令
//    if (wasDragging && draggedIndex != -1 && draggedIndex < m_elements.size()) {
//        const wxPoint& newPos = m_elements[draggedIndex].GetPos();
//        wxLogDebug("Pos: start=(%d,%d), new=(%d,%d)", startPos.x, startPos.y, newPos.x, newPos.y);
//
//        if (newPos != startPos) {
//            m_undoStack.Push(std::make_unique<CmdMoveElement>(
//                draggedIndex,
//                startPos,
//                anchors));
//            wxLogDebug("Pushed Move command, undo name: %s", m_undoStack.GetUndoName().c_str());
//
//            MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
//            if (mf) mf->OnUndoStackChanged();
//        }
//    }
//
//    // 重置拖动状态
//    m_isDragging = false;
//    evt.Skip();
//}
void CanvasPanel::OnLeftUp(wxMouseEvent& evt)
{
    // 如果定时器还在运行，说明是Pin_Input的单击，但还未确定是否要拖动
    // 我们让定时器自然触发来处理单击逻辑
    if (m_clickTimer.IsRunning()) {
        evt.Skip();
        return;
    }

    wxPoint screenpos = evt.GetPosition();
    wxPoint canvasPos = ScreenToCanvas(screenpos);

    // 保存拖动状态（元件移动用）
    bool wasDragging = m_isDragging;
    int draggedIndex = m_selectedIndex;
    wxPoint startPos = m_dragStartElemPos;
    auto anchors = m_undoAnchors;

    // 保存导线绘制状态
    bool wasDrawingWire = (m_wireMode == WireMode::DragNew);

    // 交给ToolManager处理
	wxPoint canvasPos = ScreenToCanvas(screenpos);

    EnsureFocus();

    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame && mainFrame->GetToolManager()) {
        mainFrame->GetToolManager()->OnCanvasLeftUp(canvasPos);
    }

    // 检测是否新增了导线
    if (m_wires.size() > m_wireCountBeforeOperation) {
        size_t newWireIndex = m_wires.size() - 1;
        m_undoStack.Push(std::make_unique<CmdAddWire>(newWireIndex));

        MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
        if (mf) mf->OnUndoStackChanged();
    }

    // 元件移动撤销
    if (wasDragging && draggedIndex != -1 && draggedIndex < m_elements.size()) {
        const wxPoint& newPos = m_elements[draggedIndex].GetPos();
        if (newPos != startPos) {
            m_undoStack.Push(std::make_unique<CmdMoveElement>(
                draggedIndex,
                startPos,
                anchors));

            if (mainFrame) {
                mainFrame->OnUndoStackChanged();
            }
        }
    }

    // 重置拖动状态
    m_isDragging = false;
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

    //// 原有的删除元件逻辑
    //if (evt.GetKeyCode() == WXK_DELETE && m_selectedIndex != -1) {
    //    // 删除元件前先更新连接的导线
    //    for (const auto& aw : m_movingWires) {
    //        if (aw.wireIdx >= m_wires.size()) continue;
    //        Wire& wire = m_wires[aw.wireIdx];
    //        const auto& elem = m_elements[m_selectedIndex];
    //        const auto& pins = aw.isInput ? elem.GetInputPins() : elem.GetOutputPins();
    //        if (aw.pinIdx >= pins.size()) continue;

    //        wxPoint newPin = elem.GetPos() + wxPoint(pins[aw.pinIdx].pos.x, pins[aw.pinIdx].pos.y);
    //        if (aw.ptIdx == 0)
    //            wire.pts.front().pos = newPin;
    //        else
    //            wire.pts.back().pos = newPin;
    //    }

    //    m_elements.erase(m_elements.begin() + m_selectedIndex);
    //    m_selectedIndex = -1;
    //    Refresh();
    //}
    //else {
    //    evt.Skip();
    //}
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

    // 记录撤销
    m_undoStack.Push(std::make_unique<CmdAddElement>(elem.GetName(), m_elements.size() - 1));
    /*MyLog("CanvasPanel::AddElement: <%s> total=%zu\n",
        elem.GetName().ToUTF8().data(), m_elements.size());*/
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
        /*MyLog("DRAW GREEN CELL: wire=%zu cell=%zu  pos=(%d,%d)\n",
            m_hoverCellWire, m_hoverCellIdx,
            m_hoverCellPos.x, m_hoverCellPos.y);*/
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(0, 255, 0), 1));
        dc.DrawCircle(m_hoverCellPos, 3);
    }

    // 5. 绘制文本元素 - 修改为使用 unique_ptr
    for (auto& textElem : m_textElements) {
            textElem.Draw(dc);
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

    // 修复：检查引脚是否存在
    wxPoint standardpos = pos;
    const auto& outputPins = clone.GetOutputPins();
    const auto& inputPins = clone.GetInputPins();

    if (!outputPins.empty()) {
        // 优先使用输出引脚
        Pin standardPin = outputPins[0];
        standardpos = pos - wxPoint(standardPin.pos.x, standardPin.pos.y);
    }
    else if (!inputPins.empty()) {
        // 如果没有输出引脚，使用输入引脚
        Pin standardPin = inputPins[0];
        standardpos = pos - wxPoint(standardPin.pos.x, standardPin.pos.y);
    }
    // 如果都没有引脚，直接使用原位置

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
    const int GRID_SIZE = 20;
    const int HIT_RADIUS = 8;

    // 首先将点击位置对齐到最近的网格点
    wxPoint alignedRaw(
        (raw.x / GRID_SIZE) * GRID_SIZE,
        (raw.y / GRID_SIZE) * GRID_SIZE
    );
   
    for (size_t w = 0; w < m_wires.size(); ++w) {
        const auto& wire = m_wires[w];
        for (size_t c = 0; c < wire.cells.size(); ++c) {
            const wxPoint& cell = wire.cells[c];

            // 检查是否在网格点上且与对齐后的点击位置匹配
            if (cell.x == alignedRaw.x && cell.y == alignedRaw.y) {
                // 二次验证：确保在点击半径内
                if (abs(raw.x - cell.x) <= HIT_RADIUS &&
                    abs(raw.y - cell.y) <= HIT_RADIUS) {

                    *wireIdx = w;
                    *cellIdx = c;
                    *cellPos = cell;
                    return c;
                }
            }
        }
    }

    *wireIdx = -1;
    *cellIdx = -1;
    *cellPos = wxPoint(0, 0);
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

    void CanvasPanel::CollectUndoAnchor(size_t elemIdx,
        std::vector<CmdMoveElement::Anchor>& out)
    {
        out.clear();
        if (elemIdx >= m_elements.size()) return;

        const auto& elem = m_elements[elemIdx];
        auto collect = [&](const auto& pins) {
            for (const auto& pin : pins) {
                wxPoint pinWorld = elem.GetPos() + wxPoint(pin.pos.x, pin.pos.y);
                for (size_t w = 0; w < m_wires.size(); ++w) {
                    const auto& wire = m_wires[w];
                    if (!wire.pts.empty() && wire.pts.front().pos == pinWorld)
                        out.emplace_back(CmdMoveElement::Anchor{ w, 0, pinWorld });
                    if (wire.pts.size() > 1 && wire.pts.back().pos == pinWorld)
                        out.emplace_back(CmdMoveElement::Anchor{ w, wire.pts.size() - 1, pinWorld });
                }
            }
            };
        collect(elem.GetInputPins());
        collect(elem.GetOutputPins());
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

void CanvasPanel::OnCursorTimer(wxTimerEvent& event) {
    // 转发到toolmanager
	//MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
 //   if (mainFrame && mainFrame->GetToolManager()) {
 //       mainFrame->GetToolManager()->OnCursorTimer(event);
	//}
	event.Skip();
}

void CanvasPanel::SetFocusToTextElement(int index) {
	SetFocus();
}

void CanvasPanel::OnFocus(wxFocusEvent& event) {
    m_hasFocus = true;
    MyLog("CanvasPanel: Gained focus\n");
    Refresh(); // 重绘以显示焦点状态
    event.Skip();
}

void CanvasPanel::OnKillFocus(wxFocusEvent& event) {
    m_hasFocus = false;
    MyLog("CanvasPanel: Lost focus\n");
    Refresh(); // 重绘以移除焦点状态
    event.Skip();
}

void CanvasPanel::EnsureFocus() {
    if (!m_hasFocus) {
        SetFocus();
        MyLog("CanvasPanel: Ensuring focus\n");
    }
}

int CanvasPanel::inWhichTextBox(const wxPoint& canvasPos) {
    for (size_t i = 0; i < m_textElements.size(); ++i) {
        if (m_textElements[i].Contains(canvasPos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void CanvasPanel::SetupHiddenTextCtrl() {
    if (!m_hiddenTextCtrl) {
        m_hiddenTextCtrl = new wxTextCtrl(this, wxID_ANY, "",
            wxPoint(-1000, -1000), wxSize(1, 1),
            wxTE_PROCESS_ENTER | wxTE_RICH2);
        m_hiddenTextCtrl->Hide();

        // 事件绑定由ToolManager处理
    }
}

void CanvasPanel::AttachHiddenTextCtrlToElement(int textIndex) {
    if (textIndex >= 0 && textIndex < (int)m_textElements.size()) {
        // 分离当前编辑的元素
        if (m_currentEditingTextIndex != -1 && m_currentEditingTextIndex < (int)m_textElements.size()) {
            m_textElements[m_currentEditingTextIndex].DetachHiddenTextCtrl();
            m_textElements[m_currentEditingTextIndex].StopEditing();
        }

        // 附加到新元素
        m_currentEditingTextIndex = textIndex;
        m_textElements[textIndex].AttachHiddenTextCtrl(m_hiddenTextCtrl);
        m_textElements[textIndex].StartEditing();

        // 显示隐藏TextCtrl（在正确位置）
        m_hiddenTextCtrl->Show();
        Refresh();
    }
}

void CanvasPanel::DetachHiddenTextCtrl() {
    if (m_currentEditingTextIndex != -1 && m_currentEditingTextIndex < (int)m_textElements.size()) {
        m_textElements[m_currentEditingTextIndex].DetachHiddenTextCtrl();
        m_textElements[m_currentEditingTextIndex].StopEditing();
        m_currentEditingTextIndex = -1;
    }

    m_hiddenTextCtrl->Hide();
    m_hiddenTextCtrl->SetPosition(wxPoint(-1000, -1000));
    Refresh();
}

void CanvasPanel::CreateTextElement(const wxPoint& position) {
    // 创建新文本元素
    CanvasTextElement newElem(this, "", position);
    m_textElements.push_back(std::move(newElem));

    // 立即开始编辑
    int newIndex = static_cast<int>(m_textElements.size() - 1);
    AttachHiddenTextCtrlToElement(newIndex);
}

void CanvasPanel::StartTextEditing(int index) {
    AttachHiddenTextCtrlToElement(index);
}