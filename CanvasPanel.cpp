#include "CanvasPanel.h"
#include "MainFrame.h"
#include "Wire.h"
#include <wx/dcbuffer.h>
#include "CanvasElement.h"
#include "my_log.h"
#include <wx/dcgraph.h>
#include <wx/graphics.h>
#include "CanvasTextElement.h"
#include "HandyToolKit.h"
#include "ToolStateMachine.h"

wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)
EVT_LEFT_UP(CanvasPanel::OnLeftUp)
EVT_LEFT_DCLICK(CanvasPanel::OnLeftDoubleClick)
EVT_RIGHT_DOWN(CanvasPanel::OnRightDown)
EVT_RIGHT_UP(CanvasPanel::OnRightUp)
EVT_MOTION(CanvasPanel::OnMouseMove)
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
EVT_MOUSEWHEEL(CanvasPanel::OnMouseWheel)
EVT_SET_FOCUS(CanvasPanel::OnFocus)
EVT_KILL_FOCUS(CanvasPanel::OnKillFocus)
EVT_TIMER(wxID_ANY, CanvasPanel::OnClickTimer)
wxEND_EVENT_TABLE()

CanvasPanel::CanvasPanel(MainFrame* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE),
    m_mainFrame(parent),
    m_offset(0, 0), m_isPanning(false), m_scale(1.0f),
    m_wireMode(WireMode::Idle), m_selectedIndex(-1), m_isDragging(false), m_hoverInfo{}, m_hasFocus(false),
    m_hiddenTextCtrl(nullptr),
    m_isUsingHiddenCtrl(false), m_currentEditingTextIndex(-1) {
    SetupHiddenTextCtrl();

    m_toolStateMachine = new ToolStateMachine(this);
    m_cursorTimer.Start(100);
    m_CanvasEventHandler = new CanvasEventHandler(this, m_toolStateMachine);
    m_HandyToolKit = new HandyToolKit(this, m_CanvasEventHandler);

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);
    SetFocus();
    MyLog("CanvasPanel: constructed\n");

    // 默认初始化仿真引擎（会在 InitializeSimulationEngine 中启动仿真并注册回调）
    InitializeSimulationEngine();
}

void CanvasPanel::InitializeSimulationEngine() {
    // 初始化引擎数据
    m_simEngine.Initialize(m_elements, m_wires);
    //wxLogMessage("CanvasPanel::InitializeSimulationEngine - elements=%zu wires=%zu", m_elements.size(), m_wires.size());
    // 注册回调：当仿真引擎内部状态变化时，CanvasPanel 更新对应 CanvasElement 并刷新
    m_simEngine.SetStateChangedCallback([this](size_t elemIdx, int newState) {
        if (elemIdx < m_elements.size()) {
            // 更新 CanvasElement 的显示状态（UI 层负责绘制）
            if (m_elements[elemIdx].GetOutputState() != newState) {
                m_elements[elemIdx].SetOutputState(newState);
                Refresh(); // 可以优化为只重绘受影响区域
            }
        }
        });

    if (m_simulationEnabled) {
        m_simEngine.StartSimulation();
    }
    else {
        m_simEngine.StopSimulation();
    }
}

void CanvasPanel::SyncSimulationToCanvas() {
    // 旧版：基于 GetAllElementStates。因引擎会通过回调主动推送状态，这里可作为补充同步
    auto states = m_simEngine.GetAllElementStates();
    for (size_t i = 0; i < m_elements.size() && i < states.size(); ++i) {
        if (!states[i].empty()) {
            int s = states[i][0];
            if (m_elements[i].GetOutputState() != s) {
                m_elements[i].SetOutputState(s);
            }
        }
    }
    Refresh();
}

void CanvasPanel::SyncCanvasToSimulation() {
    InitializeSimulationEngine();
}

void CanvasPanel::ClearAll() {
    m_elements.clear();
    m_wires.clear();
    m_selectedIndex = -1;
    // 重建仿真
    InitializeSimulationEngine();
    Refresh();
}

void CanvasPanel::AddWire(const Wire& wire) {
    m_wires.push_back(wire);
    InitializeSimulationEngine();
    Refresh();
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

    // 2. 保存操作前的导线数量
    m_wireCountBeforeOperation = m_wires.size();

    // 3. 交给ToolManager处理
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasLeftDown(rawCanvasPos);
    }

    // 4. 如果没有点击元件且ToolManager没处理，清除选择
    if (idx == -1 && m_CanvasEventHandler && !m_CanvasEventHandler->IsEventHandled()) {
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

//void CanvasPanel::OnLeftDoubleClick(wxMouseEvent& evt)
//{
//    wxPoint rawCanvasPos = ScreenToCanvas(evt.GetPosition());
//
//    // 停止定时器，避免单击逻辑执行
//    if (m_clickTimer.IsRunning()) {
//        m_clickTimer.Stop();
//    }
//
//    // 检查是否双击了Pin_Input或Pin_Output元件
//    int idx = HitTest(rawCanvasPos);
//    if (idx != -1 && idx < m_elements.size()) {
//        wxString elementId = m_elements[idx].GetId();
//
//        if (elementId == "Pin_Input") {
//            // 切换Pin_Input的状态
//            m_elements[idx].ToggleState();
//            Refresh();
//            m_isDragging = false;
//            evt.Skip(false);
//            return;
//        }
//        else if (elementId == "Pin_Output") {
//            // 切换Pin_Output的状态（循环切换：X->0->1->X）
//            int currentState = m_elements[idx].GetOutputState();
//            int newState = (currentState + 1) % 3; // 0->1->2->0
//            m_elements[idx].SetOutputState(newState);
//            Refresh();
//            m_isDragging = false;
//            evt.Skip(false);
//            return;
//        }
//    }
//
//    // 对于其他元件，继续正常处理
//    evt.Skip();
//}
void CanvasPanel::OnLeftDoubleClick(wxMouseEvent& evt)
{

    wxPoint rawCanvasPos = ScreenToCanvas(evt.GetPosition());
    if (m_clickTimer.IsRunning()) m_clickTimer.Stop();

    int idx = HitTest(rawCanvasPos);
    //wxLogMessage("CanvasPanel::OnLeftDoubleClick called at (%d,%d)", rawCanvasPos.x, rawCanvasPos.y);
    //wxLogMessage("  HitTest idx=%d (elements=%zu)", idx, m_elements.size());
    if (idx != -1 && idx < m_elements.size()) {
        wxLogMessage("  Element id=%s state=%d", m_elements[idx].GetId().ToUTF8().data(), m_elements[idx].GetOutputState());
    }

    if (idx != -1 && idx < m_elements.size()) {
        wxString elementId = m_elements[idx].GetId();
        if (elementId == "Pin_Input") {
            // 交由仿真引擎切换并通过回调同步 UI
            m_simEngine.ToggleInputPin(idx);
            m_isDragging = false;
            evt.Skip(false);
            return;
        }
        else if (elementId == "Pin_Output") {
            if (m_simulationEnabled) {
                // 仿真运行时输出由仿真控制，禁止手动切换；强制同步显示仿真值
                SyncSimulationToCanvas();
                m_isDragging = false;
                evt.Skip(false);
                return;
            }
            else {
                int currentState = m_elements[idx].GetOutputState();
                int newState = (currentState + 1) % 3;
                m_elements[idx].SetOutputState(newState);
                Refresh();
                m_isDragging = false;
                evt.Skip(false);
                return;
            }
        }
    }
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
	UpdateHoverInfo(rawCanvasPos);

    // 交给工具管理器处理
        // 交给工具管理器处理
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasMouseMove(rawCanvasPos);
        if (m_CanvasEventHandler->IsEventHandled()) {
            return; // 工具管理器已处理事件
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

    EnsureFocus();

    MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasLeftUp(canvasPos);
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
            wxPoint canvasPos = ScreenToCanvas(screenpos);

            if (m_CanvasEventHandler) {
                m_CanvasEventHandler->OnCanvasLeftUp(canvasPos);
                if (m_CanvasEventHandler->IsEventHandled()) {
                    return; // 工具管理器已处理事件
                }
            }

            // 重置拖动状态
            m_isDragging = false;
            evt.Skip();
        }
    }
}

void CanvasPanel::OnKeyDown(wxKeyEvent& evt) {
    // 交给工具管理器处理
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasKeyDown(evt);
        if (m_CanvasEventHandler->IsEventHandled()) {
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
    if (m_CanvasEventHandler){
        m_CanvasEventHandler->OnCanvasMouseWheel(evt);
        if (m_CanvasEventHandler->IsEventHandled()) {
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
    InitializeSimulationEngine();
    Refresh();
    m_undoStack.Push(std::make_unique<CmdAddElement>(elem.GetName(), m_elements.size() - 1));
}

//================= 绘制 =================
void CanvasPanel::OnPaint(wxPaintEvent&) {
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
        int maxX = static_cast<int>(sz.x);
        int maxY = static_cast<int>(sz.y);

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
        if (m_wireMode == WireMode::DragBranch) m_tempWire.Draw(*gcdc);

        // 4. 悬停引脚高亮（绿色空心圆）
        if (m_hoverInfo.pinIndex != -1) {
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(wxColour(0, 255, 0), 1.0 / m_scale));
            gc->DrawEllipse(m_hoverInfo.pinWorldPos.x - 3, m_hoverInfo.pinWorldPos.y - 3, 6, 6);
        }

        if (m_hoverInfo.cellIndex != -1) {
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(wxColour(0, 255, 0), 1.0 / m_scale));
            gc->DrawEllipse(m_hoverInfo.cellWorldPos.x - 3, m_hoverInfo.cellWorldPos.y - 3, 6, 6);
        }

        // 5. 绘制文本元素 - 修改为使用 unique_ptr
        for (auto& textElem : m_textElements) {
            textElem.Draw(*gcdc);
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
        int maxX = static_cast<int>(sz.x);  // 转换为画布坐标
        int maxY = static_cast<int>(sz.y);
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
        if (m_wireMode == WireMode::DragBranch) m_tempWire.Draw(dc);

        // 4. 悬停引脚：绿色空心圆
        if (m_hoverInfo.pinIndex != -1) {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);              // 不填充 → 空心
            dc.SetPen(wxPen(wxColour(0, 255, 0), 1));       // 绿色边框，线宽 2
            dc.DrawCircle(m_hoverInfo.pinWorldPos, 3);                // 半径 3 像素
        }

        if (m_hoverInfo.cellIndex != -1) {
            /*MyLog("DRAW GREEN CELL: wire=%zu cell=%zu  pos=(%d,%d)\n",
                m_hoverCellWire, m_hoverCellIdx,
                m_hoverCellPos.x, m_hoverCellPos.y);*/
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(wxPen(wxColour(0, 255, 0), 1));
            dc.DrawCircle(m_hoverInfo.cellWorldPos, 3);
        }
        // 5. 绘制文本元素 - 修改为使用 unique_ptr
        for (auto& textElem : m_textElements) {
            textElem.Draw(dc);
        }
    }
}

//================= 放置元件 =================
void CanvasPanel::PlaceElement(const wxString& name, const wxPoint& pos){
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
    wxPoint alignedRaw((raw.x + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE, (raw.y + GRID_SIZE / 2) / GRID_SIZE * GRID_SIZE);
   
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

//// 删除选中的元件及关联导线
//void CanvasPanel::DeleteSelectedElement() {
//    if (m_selectedIndex == -1) return; // 无选中元件
//
//    // 1. 清理关联的导线
//    ClearElementWires(m_selectedIndex);
//
//    // 2. 删除选中的元件
//    m_elements.erase(m_elements.begin() + m_selectedIndex);
//
//    // 3. 重置选中状态
//    m_selectedIndex = -1;
//    m_isDragging = false;
//    m_movingWires.clear();
//
//    // 4. 刷新画布
//    Refresh();
//    MyLog("CanvasPanel: Element deleted, remaining count: %zu\n", m_elements.size());
//
//}
void CanvasPanel::DeleteSelectedElement() {
    if (m_selectedIndex == -1) return;
    ClearElementWires(m_selectedIndex);
    m_elements.erase(m_elements.begin() + m_selectedIndex);
    m_selectedIndex = -1;
    m_isDragging = false;
    m_movingWires.clear();
    // 重建仿真
    InitializeSimulationEngine();
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

        if (m_CanvasEventHandler) {
            m_CanvasEventHandler->OnCanvasRightDown(rawScreenPos);
            if (m_CanvasEventHandler->IsEventHandled()) {
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
        if (m_CanvasEventHandler) {
            m_CanvasEventHandler->OnCanvasRightUp(rawScreenPos);
            if (m_CanvasEventHandler->IsEventHandled()) {
                return; // 工具管理器已处理事件
            }
        }

        evt.Skip();
    }

    void CanvasPanel::SetStatus(wxString status) {
		wxString hover = "";
        if (m_hoverInfo.IsOverPin()) {
            hover = (wxString::Format("悬停于: %sPin[%d]",
                m_hoverInfo.isInputPin ? "Input" : "Output", m_hoverInfo.pinIndex));
        }
        else if (m_hoverInfo.IsOverCell()) {
            hover = (wxString::Format("悬停于: Wire[%d] Cell[%d]",
                m_hoverInfo.wireIndex, m_hoverInfo.cellIndex));
        }
        else if (m_hoverInfo.IsOverElement()) {
            hover =(wxString::Format("悬停于: Component[%s]",
                m_hoverInfo.elementName));
        }
        else {
            hover = wxString::Format("悬停于: 无悬停对象");
        }

        wxString cursor = wxString::Format("指针位置: (%d, %d)", m_hoverInfo.pos.x, m_hoverInfo.pos.y);

        wxString zoom = wxString::Format("缩放：%d%%", int(m_scale*100));

        m_mainFrame->SetStatusText(status, 0);
        m_mainFrame->SetStatusText(cursor, 1);
        m_mainFrame->SetStatusText(hover, 2);
        m_mainFrame->SetStatusText(zoom, 3);
    }

    void CanvasPanel::SetCurrentTool(ToolType tool) {
		m_toolStateMachine->SetCurrentTool(tool);
    }

    void CanvasPanel::SetCurrentComponent(const wxString& componentName) {
        if (m_toolStateMachine->GetCurrentTool() != ToolType::COMPONENT_TOOL) {
            m_toolStateMachine->SetCurrentTool(ToolType::COMPONENT_TOOL);
		}
		m_CanvasEventHandler->SetCurrentComponent(componentName);
    }

    void CanvasPanel::UpdateHoverInfo(const wxPoint& canvasPos) {
        m_hoverInfo.pos = canvasPos;

        // 悬停引脚信息检测
        bool isInput = false;
        wxPoint pinWorldPos;
		int pinIdx = HitHoverPin(canvasPos, &isInput, &pinWorldPos);

		// 导线控制点信息检测
        int cellWire = -1;
        int cellIdx = -1;
		wxPoint cellWorldPos;
		int hitCellIdx = HitHoverCell(canvasPos, &cellWire, &cellIdx, &cellWorldPos);

		// 悬停元件信息检测
        int elementIndex = HitTestPublic(canvasPos);

        // 更新信息
        m_hoverInfo.pinIndex = pinIdx;
		m_hoverInfo.isInputPin = isInput;
		m_hoverInfo.pinWorldPos = pinWorldPos;

        m_hoverInfo.cellIndex = cellIdx;
        m_hoverInfo.wireIndex = cellWire;
		m_hoverInfo.cellWorldPos = cellWorldPos;

        m_hoverInfo.elementIndex = elementIndex;
        if (elementIndex != -1) m_hoverInfo.elementName = m_elements[elementIndex].GetName();
        else m_hoverInfo.elementName = "";
        
		Refresh(); // 触发重绘以显示悬停效果
    }

    void CanvasPanel::CollectUndoAnchor(size_t elemIdx, std::vector<WireAnchor>& out) {
        out.clear();
        if (elemIdx >= m_elements.size()) return;

        const auto& elem = m_elements[elemIdx];
        auto collect = [&](const auto& pins, bool isInput) {
            for (size_t pinIdx = 0; pinIdx < pins.size(); ++pinIdx) {
                const auto& pin = pins[pinIdx];
                wxPoint pinWorld = elem.GetPos() + wxPoint(pin.pos.x, pin.pos.y);
                for (size_t w = 0; w < m_wires.size(); ++w) {
                    const auto& wire = m_wires[w];
                    if (!wire.pts.empty() && wire.pts.front().pos == pinWorld)
                        out.emplace_back(WireAnchor{ w, 0, isInput, pinIdx });
                    if (wire.pts.size() > 1 && wire.pts.back().pos == pinWorld)
                        out.emplace_back(WireAnchor{ w, wire.pts.size() - 1, isInput, pinIdx });
                }
            }
            };

        collect(elem.GetInputPins(), true);
        collect(elem.GetOutputPins(), false);
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

void CanvasPanel::StartBranchFromWire(size_t wireIdx, size_t cellIdx, const wxPoint& startPos) {
    m_wireMode = WireMode::DragBranch;
    m_branchDragState.parentWire = wireIdx;
    m_branchDragState.parentCell = cellIdx;
    m_branchDragState.branchStartPos = startPos;

    // 初始化临时连线
    m_tempWire.Clear();
    ControlPoint cp{ startPos, CPType::Bend };
    m_tempWire.AddPoint(cp);

    Refresh();
}

void CanvasPanel::CompleteBranchConnection() {
    if (m_tempWire.Empty() || m_tempWire.pts.size() < 2) {
        m_wireMode = WireMode::Idle;
        return;
    }

    // 检查终点是否连接到有效目标（引脚或其他导线）
    wxPoint endPos = m_tempWire.pts.back().pos;

    // 这里可以检查是否连接到引脚或其他导线
    bool isInput;
    wxPoint pinPos;
    int pinIdx = HitHoverPin(endPos, &isInput, &pinPos);

    if (pinIdx != -1) {
        // 连接到引脚，完成分支创建
        Wire branchWire = m_tempWire;
        branchWire.pts.back().type = CPType::Pin;

        // 添加到导线列表
        size_t branchIdx = m_wires.size();
        m_wires.push_back(branchWire);

        // 建立分支关系
        EstablishBranchConnection(
            m_branchDragState.parentWire,
            m_branchDragState.parentCell,
            branchIdx
        );
        auto recordConnection = [&](const wxPoint& pinPos, size_t ptIdx) {
            for (size_t i = 0; i < m_elements.size(); ++i) {
                const auto& elem = m_elements[i];
                auto test = [&](const auto& pins, bool isIn) {
                    for (size_t p = 0; p < pins.size(); ++p) {
                        wxPoint w = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
                        if (w == pinPos) {
                            m_branchWires.push_back({ m_wires.size() - 1, ptIdx, isIn, p });
                            return true;
                        }
                    }
                    return false;
                    };
                if (test(elem.GetInputPins(), true)) return;
                test(elem.GetOutputPins(), false);
            }
        };
        if (branchWire.pts.front().type == CPType::Pin)
            recordConnection(branchWire.pts.front().pos, 0);
        if (branchWire.pts.back().type == CPType::Pin)
            recordConnection(branchWire.pts.back().pos, branchWire.pts.size() - 1);

        // 记录撤销操作
        m_undoStack.Push(std::make_unique<CmdAddBranchWire>(
            m_branchDragState.parentWire,
            m_branchDragState.parentCell,
            branchIdx));
    }

    // 检查是否连接到导线
    int cellWire, cellIdx;
    wxPoint cellPos;
    int newCell = HitHoverCell(endPos, &cellWire, &cellIdx, &cellPos);
    if (newCell != -1) {
        // 连接到导线，完成分支创建
        Wire branchWire = m_tempWire;
        branchWire.pts.back().type = CPType::Pin;

        // 添加到导线列表
        size_t branchIdx = m_wires.size();
        m_wires.push_back(branchWire);

        // 建立分支关系
        EstablishBranchConnection(
            m_branchDragState.parentWire,
            m_branchDragState.parentCell,
            branchIdx
        );
        auto recordConnection = [&](const wxPoint& pinPos, size_t ptIdx) {
            for (size_t i = 0; i < m_elements.size(); ++i) {
                const auto& elem = m_elements[i];
                auto test = [&](const auto& pins, bool isIn) {
                    for (size_t p = 0; p < pins.size(); ++p) {
                        wxPoint w = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
                        if (w == pinPos) {
                            m_branchWires.push_back({ m_wires.size() - 1, ptIdx, isIn, p });
                            return true;
                        }
                    }
                    return false;
                    };
                if (test(elem.GetInputPins(), true)) return;
                test(elem.GetOutputPins(), false);
            }
            };
        if (branchWire.pts.front().type == CPType::Pin)
            recordConnection(branchWire.pts.front().pos, 0);
        if (branchWire.pts.back().type == CPType::Pin)
            recordConnection(branchWire.pts.back().pos, branchWire.pts.size() - 1);

        // 记录撤销操作
        m_undoStack.Push(std::make_unique<CmdAddBranchWire>(
            m_branchDragState.parentWire,
            m_branchDragState.parentCell,
            branchIdx));
    }

    if (newCell == -1 && pinIdx == -1) {
        // 连接到自由点，完成分支创建
        Wire branchWire = m_tempWire;
        branchWire.pts.back().type = CPType::Free;

        // 添加到导线列表
        size_t branchIdx = m_wires.size();
        m_wires.push_back(branchWire);

        // 建立分支关系
        EstablishBranchConnection(
            m_branchDragState.parentWire,
            m_branchDragState.parentCell,
            branchIdx
        );
        auto recordConnection = [&](const wxPoint& pinPos, size_t ptIdx) {
            for (size_t i = 0; i < m_elements.size(); ++i) {
                const auto& elem = m_elements[i];
                auto test = [&](const auto& pins, bool isIn) {
                    for (size_t p = 0; p < pins.size(); ++p) {
                        wxPoint w = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
                        if (w == pinPos) {
                            m_branchWires.push_back({ m_wires.size() - 1, ptIdx, isIn, p });
                            return true;
                        }
                    }
                    return false;
                    };
                if (test(elem.GetInputPins(), true)) return;
                test(elem.GetOutputPins(), false);
            }
            };
        if (branchWire.pts.front().type == CPType::Pin)
            recordConnection(branchWire.pts.front().pos, 0);
        if (branchWire.pts.back().type == CPType::Pin)
            recordConnection(branchWire.pts.back().pos, branchWire.pts.size() - 1);

        // 记录撤销操作
        m_undoStack.Push(std::make_unique<CmdAddBranchWire>(
            m_branchDragState.parentWire,
            m_branchDragState.parentCell,
            branchIdx));
    }

    // 新增：在所有分支创建路径结束后，重建仿真
    InitializeSimulationEngine();

    m_wireMode = WireMode::Idle;
    m_tempWire.Clear();
    Refresh();
}

void CanvasPanel::EstablishBranchConnection(size_t parentWire, size_t parentCell, size_t branchWire) {
    if (parentWire >= m_wires.size() || branchWire >= m_wires.size()) return;

    Wire& parent = m_wires[parentWire];
    Wire& branch = m_wires[branchWire];

    // 建立分支关系
    WireBranch newBranch{ parentWire, parentCell, branchWire };
    parent.branches.push_back(newBranch);

    // 标记分支导线
    branch.isBranch = true;
    branch.parentWire = parentWire;
    branch.parentCell = parentCell;

    // 添加到全局分支列表
    m_allBranches.push_back(newBranch);

    Refresh();
}

void CanvasPanel::RemoveBranchConnection(size_t parentWire, size_t branchWire) {
    if (parentWire >= m_wires.size() || branchWire >= m_wires.size()) return;

    Wire& parent = m_wires[parentWire];
    Wire& branch = m_wires[branchWire];

    // 从父导线移除分支
    auto it = std::find_if(parent.branches.begin(), parent.branches.end(),
        [branchWire](const WireBranch& wb) { return wb.branchWire == branchWire; });

    if (it != parent.branches.end()) {
        parent.branches.erase(it);
    }

    // 重置分支标记
    branch.isBranch = false;
    branch.parentWire = -1;
    branch.parentCell = -1;

    // 从全局分支列表移除
    auto globalIt = std::find_if(m_allBranches.begin(), m_allBranches.end(),
        [branchWire](const WireBranch& wb) { return wb.branchWire == branchWire; });

    if (globalIt != m_allBranches.end()) {
        m_allBranches.erase(globalIt);
    }

    Refresh();
}

//// 删除导线时同时删除其所有分支
//void CanvasPanel::DeleteWireWithBranches(size_t wireIdx) {
//    if (wireIdx >= m_wires.size()) return;
//
//    Wire& wire = m_wires[wireIdx];
//
//    // 递归删除所有分支
//    for (const auto& branch : wire.branches) {
//        DeleteWireWithBranches(branch.branchWire);
//    }
//
//    // 如果是分支，从父导线移除
//    if (wire.isBranch && wire.parentWire < m_wires.size()) {
//        Wire& parent = m_wires[wire.parentWire];
//        auto it = std::find_if(parent.branches.begin(), parent.branches.end(),
//            [wireIdx](const WireBranch& wb) { return wb.branchWire == wireIdx; });
//
//        if (it != parent.branches.end()) {
//            parent.branches.erase(it);
//        }
//    }
//
//    // 删除导线本身
//    m_wires.erase(m_wires.begin() + wireIdx);
//
//    // 更新后续导线的索引（如果需要）
//    // 这里可能需要更复杂的索引更新逻辑
//
//    Refresh();
//}
void CanvasPanel::DeleteWireWithBranches(size_t wireIdx) {
    if (wireIdx >= m_wires.size()) return;
    Wire& wire = m_wires[wireIdx];
    for (const auto& branch : wire.branches) {
        DeleteWireWithBranches(branch.branchWire);
    }
    if (wire.isBranch && wire.parentWire < m_wires.size()) {
        Wire& parent = m_wires[wire.parentWire];
        auto it = std::find_if(parent.branches.begin(), parent.branches.end(),
            [wireIdx](const WireBranch& wb) { return wb.branchWire == wireIdx; });
        if (it != parent.branches.end()) parent.branches.erase(it);
    }
    m_wires.erase(m_wires.begin() + wireIdx);
    // 重建仿真
    InitializeSimulationEngine();
    Refresh();
}